#include <netinet/in.h>

#include <chrono>
#include <format>
#include <optional>

#include "thread_safe_queue.hpp"

class Client {
 public:
  [[nodiscard]] auto socket() const -> Socket const& { return socket_; }
  [[nodiscard]] auto name() const -> std::string { return name_; }

  Client(Socket socket, std::string const& name) : socket_(std::move(socket)), name_(name) {}

  bool operator==(Client const& other) const noexcept { return socket_ == other.socket_; }

 private:
  Socket socket_;
  std::string name_;
};

struct Message {
  std::string data;
  std::string sender_name;
};

TSQueue<Message> static g_message_queue;
std::vector<std::unique_ptr<Client>> static g_client_list;
std::mutex static g_client_list_mutex;

auto static setupServer(std::string const& port) -> Socket {
  addrinfo const info_hints = [] {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    return hints;
  }();
  auto addr_info_or_err = AddrInfo::make(nullptr, port.c_str(), &info_hints);
  if (!addr_info_or_err.has_value()) {
    std::println(stderr, "Failed to set socket options");
    std::println(stderr, "{}", gai_strerror(addr_info_or_err.error()));
    exit(EXIT_FAILURE);
  }
  AddrInfo const& addr_info = *addr_info_or_err;
  Socket server{socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol)};
  int const server_fd = server.get();
  if (server_fd == -1) {
    std::println(stderr, "Failed to create socket");
    perror("socket");
    exit(EXIT_FAILURE);
  }
  if (bind(server_fd, addr_info->ai_addr, addr_info->ai_addrlen) == -1) {
    std::println("Failed to bind socket");
    perror("bind");
    exit(EXIT_FAILURE);
  }
  return server;
}

auto static setupClientConnection(int server_fd) -> std::optional<Socket> {
  sockaddr_in client_addr{};
  socklen_t client_addr_len = sizeof(client_addr);
  Socket client{accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len)};
  if (client.get() == -1) {
    std::println(stderr, "Failed to accept connection");
    perror("accept");
    return std::nullopt;
  }
  return client;
}

void static pushClientMessages(Client const* client) {
  while (true) {
    auto const client_message = nc::receive(client->socket());
    if (client_message) {
      Message message{*client_message, client->name()};
      g_message_queue.push(std::move(message));
    } else {
      switch (client_message.error()) {
        case ReceiveError::Disconnected: {
          std::println("Client disconnected: {}", client->name());
          std::scoped_lock const lock{g_client_list_mutex};
          auto client_iter =
            std::ranges::find_if(g_client_list, [&](std::unique_ptr<Client> const& current_client) {
              return current_client.get() == client;
            });
          g_client_list.erase(client_iter);
          return;
        }
        case ReceiveError::Other:
          break;
      }
    }
  }
}

auto static sendWithRetry(std::string const& message_to_send, Client const* client) -> void {
  int constexpr max_retries = 5;
  for (int retries = 0; retries < max_retries; retries++) {
    if (auto error = nc::send(client->socket(), message_to_send);
        !error.has_value() || error->get() == EPIPE || error->get() == ECONNRESET) {
      break;
    }
    std::println(stderr, "Failed to send message to: {}, retrying...", client->name());
  }
}

[[noreturn]] void static popClientMessages() {
  while (true) {
    auto [client_message, client_name] = g_message_queue.pop();
    std::string const message_to_send = std::format(
      "[{}, {:%F %R}] {}", client_name, std::chrono::system_clock::now(), client_message);
    std::scoped_lock const lock{g_client_list_mutex};
    for (auto const& client : g_client_list) {
      sendWithRetry(message_to_send, client.get());
    }
  }
}

int main(int argc, char** argv) {
  std::string const port = argc > 1 ? argv[1] : "4286";
  Socket const server = setupServer(port);
  if (listen(server.get(), SOMAXCONN) == -1) {
    std::println("Failed to listen on socket");
    perror("listen");
    return EXIT_FAILURE;
  }

  std::jthread const message_popper{popClientMessages};

  while (true) {
    if (auto client_socket = setupClientConnection(server.get())) {
      auto name = nc::receive(*client_socket);
      if (name) {
        std::println("Client connected: {}", *name);
      } else {
        continue;
      }
      std::scoped_lock const lock{g_client_list_mutex};
      g_client_list.emplace_back(std::make_unique<Client>(std::move(*client_socket), *name));
      std::jthread client_handler{pushClientMessages, g_client_list.back().get()};
      client_handler.detach();
    }
  }
  return EXIT_SUCCESS;
}
