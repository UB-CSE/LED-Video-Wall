#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "command.hpp"

using namespace testing;

TEST(CommandParser, Empty) {
  std::string_view input;

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());

  input = "    ";
  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, Basic) {
  std::string_view input = "mycommand arg1 arg2  arg3  ";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("mycommand"));
  ASSERT_THAT(input, StrEq("arg1 arg2  arg3  "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg1"));
  ASSERT_THAT(input, StrEq("arg2  arg3  "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg2"));
  ASSERT_THAT(input, StrEq(" arg3  "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg3"));
  ASSERT_THAT(input, StrEq(" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, SingleQuotes) {
  std::string_view input = "mycommand arg1\t'long \"argument\"' ";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("mycommand"));
  ASSERT_THAT(input, StrEq("arg1\t'long \"argument\"' "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg1"));
  ASSERT_THAT(input, StrEq("'long \"argument\"' "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("long \"argument\""));
  ASSERT_THAT(input, StrEq(" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, SingleQuotesEmpty) {
  std::string_view input = "''";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq(""));
  ASSERT_THAT(input, StrEq(""));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, DoubleQuotes) {
  std::string_view input = "mycommand arg1\t\"long 'argument'\" ";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("mycommand"));
  ASSERT_THAT(input, StrEq("arg1\t\"long 'argument'\" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("arg1"));
  ASSERT_THAT(input, StrEq("\"long 'argument'\" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq("long 'argument'"));
  ASSERT_THAT(input, StrEq(" "));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, DoubleQuotesEmpty) {
  std::string_view input = "\"\"";

  std::optional<std::string_view> token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsTrue());
  ASSERT_THAT(token.value(), StrEq(""));
  ASSERT_THAT(input, StrEq(""));

  token = CommandParser::NextToken(input);
  ASSERT_THAT(token.has_value(), IsFalse());
}

TEST(CommandParser, BadQuotes) {
  {
    std::string_view input = "mycommand arg1\t\"long 'argument' ";

    std::optional<std::string_view> token = CommandParser::NextToken(input);
    ASSERT_THAT(token.has_value(), IsTrue());
    ASSERT_THAT(token.value(), StrEq("mycommand"));
    ASSERT_THAT(input, StrEq("arg1\t\"long 'argument' "));

    token = CommandParser::NextToken(input);
    ASSERT_THAT(token.has_value(), IsTrue());
    ASSERT_THAT(token.value(), StrEq("arg1"));
    ASSERT_THAT(input, StrEq("\"long 'argument' "));

    token = CommandParser::NextToken(input);
    ASSERT_THAT(token.has_value(), IsFalse());
  }

  {
    std::string_view input = "'";

    std::optional<std::string_view> token = CommandParser::NextToken(input);
    ASSERT_THAT(token.has_value(), IsFalse());
  }
}

TEST(CommandParser, ToArgs) {
  std::string_view input = "mycommand arg1\t\"long 'argument'\" arg3\r\n";
  std::string_view command;
  std::vector<std::string_view> args;

  bool result = CommandParser::ToArgs(input, command, args);
  ASSERT_THAT(result, IsTrue());
  ASSERT_THAT(command, StrEq("mycommand"));
  ASSERT_THAT(args, ElementsAre("arg1", "long 'argument'", "arg3"));
}

TEST(CommandParser, ToArgsNoCommand) {
  std::string_view input = "   ";
  std::string_view command;
  std::vector<std::string_view> args;

  bool result = CommandParser::ToArgs(input, command, args);
  ASSERT_THAT(result, IsFalse());
}
