#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>

auto static connectToServer(
  std::string const& host,
  std::string const& port) // not using a std::string_view because old functions need .c_str()
  -> std::optional<Socket> {
  addrinfo const addr_hint = [] {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    return hints;
  }();
  auto addr_info_or_error = AddrInfo::make(host.c_str(), port.c_str(), &addr_hint);
  if (!addr_info_or_error.has_value()) {
    std::println(stderr, "getaddrinfo error: {}", gai_strerror(addr_info_or_error.error()));
    return std::nullopt;
  }
  auto const& addr_info = *addr_info_or_error;
  Socket sock_fd{socket(addr_info->ai_family, addr_info->ai_socktype, addr_info->ai_protocol)};
  if (sock_fd.get() == -1) {
    perror("socket error");
    return std::nullopt;
  }
  if (connect(sock_fd.get(), addr_info->ai_addr, addr_info->ai_addrlen) == -1) {
    perror("connect error: ");
    return std::nullopt;
  }
  return sock_fd;
}

auto static getlineNonBlock() -> std::optional<std::string> {
  if (bool static setup_done = false; !setup_done) {
    int const flags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    setup_done = true;
  }
  assert(isatty(STDIN_FILENO) && "This function is only intended to be used with a terminal");
  std::string line(128, '\0');
  ssize_t bytes_read = 0;
  size_t total_bytes_read = 0;
  while (true) {
    bytes_read = read(STDIN_FILENO, line.data() + total_bytes_read, line.size() - total_bytes_read);
    if ((bytes_read == -1 && errno == EAGAIN) || bytes_read == 0) {
      return std::nullopt;
    }
    if (bytes_read == -1) {
      perror("read error");
      return std::nullopt;
    }
    total_bytes_read += static_cast<size_t>(bytes_read);
    if (total_bytes_read >= line.size()) {
      line.resize(line.size() * 2);
    } else if (!line.contains('\n')) {
      continue;
    } else {
      break;
    }
  }
  line.resize(total_bytes_read);
  line.back() = '\0';
  return line;
}

auto static readTerminalAndSend(
  std::stop_token token /* NOLINT(performance-unnecessary-value-param) */, Socket const& sock_fd)
  -> void {
  while (!token.stop_requested()) {
    std::optional<std::string> const client_message = getlineNonBlock();
    if (!client_message) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }
    if (auto error = nc::send(sock_fd, *client_message)) {
      std::println(stderr, "Error sending: {}", error->message());
    }
  }
}

int main(int argc, char** argv) {
  std::string const ip_address = argc >= 2 ? argv[1] : "127.0.0.1";
  std::string const port = argc >= 3 ? argv[2] : "4286";

  auto const server_sock = connectToServer(ip_address, port);
  if (!server_sock) {
    std::println(stderr, "Failed to connect to server");
    return EXIT_FAILURE;
  }
  bool name_sent = false;
  while (!name_sent) {
    std::print("Enter your name: ");
    std::string name;
    std::getline(std::cin, name);
    if (auto error = nc::send(*server_sock, name)) {
      std::println(stderr, "Error sending: {}", error->message());
      continue;
    }
    name_sent = true;
  }

  std::jthread sender{readTerminalAndSend, std::ref(*server_sock)};

  // Receive response from server
  while (true) {
    auto response = nc::receive(*server_sock);
    if (response) {
      std::println("Message from server: {}", *response);
    } else {
      switch (response.error()) {
        case ReceiveError::Disconnected:
          std::println("Server closed the connection.");
          sender.request_stop();
          return EXIT_SUCCESS;
        case ReceiveError::Other:
          continue;
      }
    }
  }
  return EXIT_SUCCESS;
}
