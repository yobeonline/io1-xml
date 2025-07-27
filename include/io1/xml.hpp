#pragma once

#include <array>
#include <type_traits>
#include <string_view>
#include <iostream>
#include <cassert>
#include <utility>

namespace io1::xml
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

  struct tree
  {
    explicit tree(std::string_view name) noexcept:name(name) {}
    std::string_view name;
  };

  namespace details
  {
    template<char C, unsigned N>
    struct indent_string {
      std::array<char, N+1> value{};

      constexpr indent_string() noexcept {
        for (std::size_t i = 0; i < N; ++i) value[i] = C;
        value[N] = '\0';
      }

      constexpr char const * c_str() const noexcept { return value.data(); }
    };

    template<char C, unsigned N> constexpr auto indent = indent_string<C, N>{};

    struct done {};

    template <char indent_char, unsigned indent_increment, unsigned indent_size>
    struct tree_impl;
      
    template <char indent_char, unsigned indent_increment, unsigned indent_size, bool tree_tag = false,
              char closing = '/'>
    struct tag_impl
    {
      explicit tag_impl(std::ostream & stream, std::string_view name) noexcept:
      stream_(&stream), name_(name)
      {
        (*stream_) << indent<indent_char, indent_size>.c_str() << '<' << name_;
      }

      tag_impl(tag_impl && t) noexcept:
      empty_(t.empty_), stream_(std::exchange(t.stream_, nullptr)), name_(t.name_)
      {}

      ~tag_impl() noexcept
      {
        if (!stream_) return;
        if (empty_)
        { (*stream_) << ' ' << closing << ">\n";
        }
        else
        {
          if constexpr (tree_tag)
          {
            (*stream_) << indent<indent_char, indent_size>.c_str() << '<' << closing << name_ << ">\n";
          }
          else
          { (*stream_) << '<' << closing << name_ << ">\n";
          }
        }
      }

      template<typename T> tag_impl & operator<<(attr<T> const & a) noexcept
      {
        assert(stream_ && "Don't use a moved from object");
        (*stream_) << ' ' << a.name << "=\"" << a.value << '\"';
        return *this;
      }

      tree_impl<indent_char, indent_increment, indent_size + indent_increment>
      operator<<(tree const & t) noexcept
      {
        assert(stream_ && "Don't use a moved from object");
        if (empty_)
        { (*stream_) << ">\n";
          empty_ = false;
        }

        return tree_impl<indent_char, indent_increment, indent_size + indent_increment>{*stream_, t.name};
      }
        
      tag_impl<indent_char, indent_increment, indent_size + indent_increment> operator<<(tag const & t) noexcept
      {
        assert(stream_ && "Don't use a moved from object");
        if (empty_)
        {
          (*stream_) << ">\n";
          empty_ = false;
        }

        return tag_impl<indent_char, indent_increment, indent_size + indent_increment>{*stream_, t.name};
      }

      template <typename T>
      done operator<<(T const & value) noexcept
      {
        assert(stream_ && "Don't use a moved from object");
        (*stream_) << '>' << value;
        empty_ = false;

        return {};
      }

      bool empty_{true};
      std::ostream * stream_;
      std::string_view name_;
    };

    template<char indent_char, unsigned indent_increment, unsigned indent_size=0>
    struct tree_impl
    {
      explicit tree_impl(std::ostream & stream, std::string_view name) noexcept:
      stream_(stream), name_(name), tag_(stream, name)
      {}

      ~tree_impl() noexcept =default;

      tree_impl(tree_impl && t) noexcept =default;

      template<typename T> tree_impl && operator<<(attr<T> const & a) && noexcept
      {
        tag_ << a;
        return std::move(*this);
      }

      auto operator<<(tag const & t) & noexcept
      {
        return tag_ << t;
      }

      auto operator<<(tree const & t) & noexcept
      {
        return tag_ << t;
      }

      std::ostream & stream_;
      std::string_view name_;
      tag_impl<indent_char, indent_increment, indent_size, true> tag_; // it is a special tag in the way that it closes on a new line
    };

    template<char indent_char, unsigned indent_increment, unsigned indent_size=0> struct prolog_impl
    {
      explicit prolog_impl(std::ostream & stream, std::string_view encoding, bool standalone) noexcept
      {
        tag_impl<indent_char, indent_increment, indent_size, false, '?'>(stream, "?xml")
          << attr("version", "1.0")
          << attr("encoding", encoding)
          << attr("standalone", (standalone ? "yes" : "no"));
      }

      prolog_impl(prolog_impl &&) noexcept {}
    };

    template<char indent_char=' ', unsigned indent_increment=2>
    struct doc
    {
      explicit doc(std::ostream & stream,
        std::string_view root_name,
        std::string_view encoding = "UTF - 8",
        bool standalone=true
      ) noexcept:
      prolog_(stream, encoding, standalone),
      root_(stream, root_name)
      {}

      doc(doc && d) noexcept:
        prolog_(std::move(d.prolog_)),
        root_(std::move(d.root_))
      {}

      template<typename T> doc && operator<<(attr<T> const & a) && noexcept
      {
        std::move(root_) << a;
        return std::move(*this);
      }

      auto operator<<(tag const & t) & noexcept
      {
        return root_ << t;
      }

      auto operator<<(tree const & t) & noexcept
      {
        return root_ << t;
      }

      prolog_impl<indent_char, indent_increment> prolog_;
      tree_impl<indent_char, indent_increment> root_;
    };
  }

  template<char indent_char=' ', unsigned indent_inc=2> using doc = details::doc<indent_char,indent_inc>;
}