#pragma once

#include <array>
#include <type_traits>
#include <string_view>
#include <iostream>
#include <cassert>
#include <utility>

namespace io1
{
  template<typename T>
  struct attr
  {
    explicit attr(std::string_view name, T const &value) noexcept:name(name), value(value) {}
    std::string_view name;
    T const &value;
  };

  struct tag
  {
    explicit tag(std::string_view name) noexcept:name(name) {}
    std::string_view name;
  };

  namespace details
  {
    template<char C, unsigned N>
    struct indent_string {
      std::array<char, N+1> value{};

      constexpr indent_string() {
        for (std::size_t i = 0; i < N; ++i) value[i] = C;
        value[N] = '\0';
      }

      constexpr char const * c_str() const noexcept { return value.data(); }
    };

    template<char C, unsigned N> constexpr auto indent = indent_string<C, N>{};

    struct xml_done {};

    template <char indent_char, unsigned indent_increment, unsigned indent_size>
    struct xml_tree;
      
    template <char indent_char, unsigned indent_size, bool tree_tag = false, char closing = '/'>
    struct xml_tag
    {
      explicit xml_tag(std::ostream & stream, std::string_view name) noexcept:
      stream_(&stream), name_(name)
      {
        (*stream_) << '\n' << indent<indent_char, indent_size>.c_str() << '<' << name_;
      }

      xml_tag(xml_tag && t) noexcept:
      empty_(t.empty_), stream_(std::exchange(t.stream_, nullptr)), name_(std::move(t.name_))
      {}

      ~xml_tag() noexcept
      {
        if (!stream_) return;
        if (empty_)
        {
          (*stream_) << ' ' << closing << '>';
        }
        else
        {
          if constexpr (tree_tag)
          {
            (*stream_) << '\n' << indent<indent_char, indent_size>.c_str() << '<' << closing << name_ << '>';
          }
          else
          {
            (*stream_) << '<' << closing << name_ << '>';
          }
        }
      }

      template<typename T> xml_tag & operator<<(attr<T> const & a) noexcept
      {
        assert(stream_ && "Don't use a moved from object");
        (*stream_) << ' ' << a.name << "=\"" << a.value << '\"';
        return *this;
      }

      template<typename T> xml_done operator<<(T const & value) noexcept
      {
        assert(stream_ && "Don't use a moved from object");
        (*stream_) << '>' << value;
        empty_ = false;

        return {};
      }

      template <char indent_char, unsigned indent_increment, unsigned indent_size>
      auto operator<<(xml_tree<indent_char, indent_increment, indent_size> tree) noexcept
      {
        assert(stream_ && "Don't use a moved from object");
        if (empty_)
        {
          (*stream_) << '>';
          empty_ = false;
        }
        return std::move(tree);
      }
      

      bool empty_{true};
      std::ostream * stream_;
      std::string_view name_;
    };

    enum class xml_ver { xml10, xml11};
    enum class xml_enc { UTF_8 };

    std::ostream & operator<<(std::ostream & stream, xml_ver version) noexcept
    {
      switch (version)
      {
        case xml_ver::xml10:
          stream << "1.0\"";
          break;
        case xml_ver::xml11:
          stream << "1.1\"";
          break;
        default:
          assert(false);
      }
      return stream;
    }

    std::ostream & operator<<(std::ostream & stream, xml_enc encoding) noexcept
    {
      switch (encoding)
      {
        case xml_enc::UTF_8:
          stream << "UTF-8\"";
          break;
        default:
          assert(false);
      }
      return stream;
    }

    template<char indent_char, unsigned indent_increment, unsigned indent_size=0>
    struct xml_tree
    {
      explicit xml_tree(std::ostream & stream, std::string_view name) noexcept:
      stream_(stream), name_(name), tag_(stream, name)
      {}

      ~xml_tree() noexcept =default;

      xml_tree(xml_tree && t) noexcept =default;

      auto tree(std::string_view name) noexcept
      {
        return tag_ << xml_tree<indent_char, indent_increment, indent_size+indent_increment>(stream_, name);
      }

      template<typename T> xml_tree && operator<<(attr<T> const & a) && noexcept
      {
        tag_ << a;
        return std::move(*this);
      }

      auto operator<<(tag const & t) & noexcept
      {
        if (tag_.empty_) stream_ << '>';
        tag_.empty_ = false;
        return xml_tag<indent_char, indent_size+indent_increment>(stream_, t.name);
      }

      std::ostream & stream_;
      std::string_view name_;
      xml_tag<indent_char, indent_size, true> tag_; // it is a special tag in the way that it closes on a new line
    };

    template<char indent_char, unsigned indent_size=0> struct xml_prolog
    {
      explicit xml_prolog(std::ostream & stream, xml_ver version, xml_enc encoding, bool standalone) noexcept
      {
        xml_tag<indent_char, indent_size, false, '?'>(stream, "?xml")
          << attr("version", version)
          << attr("encoding", encoding)
          << attr("standalone", (standalone ? "yes" : "no"));
      }

      xml_prolog(xml_prolog &&) noexcept {}
    };

    template<char indent_char=' ', unsigned indent_increment=2>
    struct xml_doc
    {
      explicit xml_doc(std::ostream & stream,
        std::string_view root_name,
        xml_ver version=xml_ver::xml11,
        xml_enc encoding=xml_enc::UTF_8,
        bool standalone=true
      ) noexcept:
      prolog_(stream, version, encoding, standalone),
      root_(stream, root_name)
      {}

      xml_doc(xml_doc && d) noexcept:
        prolog_(std::move(d.prolog_)),
        root_(std::move(d.root_))
      {}

      template<typename T> xml_doc && operator<<(attr<T> const & a) && noexcept
      {
        std::move(root_) << a;
        return std::move(*this);
      }

      auto tree(std::string_view name) noexcept
      {
        return root_.tree(name);
      }

      auto operator<<(tag const & t) & noexcept
      {
        return root_ << t;
      }

      xml_prolog<indent_char> prolog_;
      xml_tree<indent_char, indent_increment> root_;
    };
  }

  template<char indent_char=' ', unsigned indent_inc=2> using xml_doc = details::xml_doc<indent_char,indent_inc>;

}