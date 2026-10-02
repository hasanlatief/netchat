#ifndef NETCHAT_SEND_HPP
#define NETCHAT_SEND_HPP

#include <optional>
#include <string>
#include "socket_raii.h"
#include "strong_typedefs.hpp"

namespace nc {
  [[nodiscard]] auto send(Socket const& sock, std::string_view msg) -> std::optional<Errno>;
}

#endif // NETCHAT_SEND_HPP
