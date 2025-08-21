#include "io1/xml.hpp" // Replace with actual header name

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <sstream>

using namespace io1::xml;

auto getlines(std::istream& stream) noexcept
{
  std::vector<std::string> lines;

  for (std::string line; std::getline(stream, line); )
  {
    lines.push_back(std::move(line));
  }

  return lines;
}

TEST_CASE("Empty default xml")
{
  std::stringstream ss;
  doc(ss, "root");

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty not standalone xml")
{
  std::stringstream ss;
  doc(ss, "root", config{.standalone=false});

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty not utf-8 xml")
{
  std::stringstream ss;
  doc(ss, "root", config{.encoding="latin-9"});

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="latin-9" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty not utf-8 and not standalone xml")
{
  std::stringstream ss;
  doc(ss, "root", config{.encoding="latin-9", .standalone=false});

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="latin-9" standalone="no" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty utf-8 and standalone xml")
{
  std::stringstream ss;
  doc(ss, "root", config{.encoding="UTF-8", .standalone=true});

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root />)");
}

TEST_CASE("Empty with attributes")
{
  std::stringstream ss;
  doc(ss, "root") << attr("lang", "en&fr");

  auto const lines = getlines(ss);
  CHECK(lines.size() == 2);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root lang="en&amp;fr" />)");
}

TEST_CASE("Top-level tags")
{
  std::stringstream ss;
  {
    auto d = doc(ss, "root");
    d << tag("foo");
  }

  auto const lines = getlines(ss);
  CHECK(lines.size() == 4);
  CHECK(lines[0] == R"(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>)");
  CHECK(lines[1] == R"(<root>)");
  CHECK(lines[2] == R"(  <foo />)");
  CHECK(lines[3] == R"(</root>)");
}
