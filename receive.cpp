#include "receive.hpp"

auto nc::receive(Socket const& socket) -> std::expected<std::string, ReceiveError> {
  using enum ReceiveError;
  uint32_t bytes_to_receive = 0;
  if (auto err = recv(socket.get(), &bytes_to_receive, sizeof(bytes_to_receive), 0); err <= 0) {
    if (err == 0) {
      return std::unexpected{Disconnected};
    }
    return std::unexpected{Other};
  }
  std::string output(bytes_to_receive, '\0');
  uint32_t total_bytes_received = 0;
  while (total_bytes_received < bytes_to_receive) {
    auto const bytes_received = recv(socket.get(), output.data(), bytes_to_receive, 0);
    if (bytes_received < 0) {
      perror("recv");
      return std::unexpected{Other};
    }
    if (bytes_received == 0) {
      return std::unexpected{Disconnected};
    }
    total_bytes_received += static_cast<uint32_t>(bytes_received);
  }
  return output;
}
