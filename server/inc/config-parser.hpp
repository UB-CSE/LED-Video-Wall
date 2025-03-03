#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "client.hpp"

#include <string>
#include <vector>

std::vector<Client*> parse_config_throws(std::string file);

#endif
