#ifndef NETCHAT_RECEIVE_HPP
#define NETCHAT_RECEIVE_HPP
#include <expected>
#include <string>

#include "socket_raii.h"

enum class ReceiveError : uint8_t { Disconnected, Other };

namespace nc {

  [[nodiscard]] auto receive(Socket const& socket) -> std::expected<std::string, ReceiveError>;
} // namespace nc

#endif // NETCHAT_RECEIVE_HPP
