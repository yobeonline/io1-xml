#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>

namespace io1::xml
{

  template <typename T>
  struct attr
  {
    explicit constexpr attr(std::string_view name, T const & value) noexcept : name(name), value(value) {}
    std::string_view name;
    T const & value;
  };

  struct tag
  {
    explicit constexpr tag(std::string_view name) noexcept : name(name) {}
    std::string_view name;
  };

  struct tree
  {
    constexpr explicit tree(std::string_view name) noexcept : name(name) {}
    std::string_view name;
  };

  namespace details
  {
    template <char indent_char, unsigned indent_increment, unsigned indent_size>
    struct indentation
    {
      using increased_t = indentation<indent_char, indent_increment, indent_size + indent_increment>;

      constexpr static auto indent() noexcept
      {
        constexpr static auto const indent_array = []
        {
          std::array<char, indent_size + 1> values;

          for (auto & c : values) c = indent_char;
          values[indent_size] = '\0';

          return values;
        }();

        return indent_array.data();
      }
    };

    template <typename T>
    concept string_like = std::convertible_to<T, std::string_view>;

    struct text_writer
    {
      std::string_view text;

      friend std::ostream & operator<<(std::ostream & stream, text_writer const & t) noexcept
      {
        for (char c : t.text)
        {
          switch (c)
          {
          case '&': stream << "&amp;"; break;
          case '"': stream << "&quot;"; break;
          case '<': stream << "&lt;"; break;
          case '>': stream << "&gt;"; break;
          default: stream << c; break;
          }
        }

        return stream;
      }
    };

    template <typename T>
      requires(!string_like<T>)
    auto write(T const & value) noexcept
    {
      return value;
    }

    template <string_like T>
    auto write(T const & value) noexcept
    {
      return text_writer{value};
    }

    struct done
    {
    };

    template <class indentation>
    class tree_impl;

    template <class indentation, bool tree_tag = false, char closing = '/'>
    class tag_impl : private indentation
    {
    public:
      explicit tag_impl(std::ostream & stream, std::string_view name) noexcept : stream_(stream), name_(name)
      {
        stream_ << indentation::indent() << '<' << name_;
      }

      constexpr tag_impl(tag_impl && other) noexcept : stream_(other.stream_), name_(other.name_)
      {
        other.active_ = false;
      }

      ~tag_impl() noexcept
      {
        if (!active_) return;
        if (empty_) { stream_ << ' ' << closing << ">\n"; }
        else
        {
          if constexpr (tree_tag) { stream_ << indentation::indent() << '<' << closing << name_ << ">\n"; }
          else { stream_ << '<' << closing << name_ << ">\n"; }
        }
      }

      template <typename T>
      tag_impl & operator<<(attr<T> const & a) noexcept
      {
        stream_ << ' ' << a.name << "=\"" << write(a.value) << '\"';
        return *this;
      }

      auto operator<<(tree const & t) noexcept
      {
        if (empty_)
        {
          stream_ << ">\n";
          empty_ = false;
        }

        return tree_impl<typename indentation::increased_t>{stream_, t.name};
      }

      auto operator<<(tag const & t) noexcept
      {
        if (empty_)
        {
          stream_ << ">\n";
          empty_ = false;
        }

        return tag_impl<typename indentation::increased_t>{stream_, t.name};
      }

      template <typename T>
      done operator<<(T const & value) noexcept
      {
        stream_ << '>' << write(value);
        empty_ = false;

        return {};
      }

    private:
      bool active_{true};
      bool empty_{true};
      std::ostream & stream_;
      std::string_view name_;
    };

    template <class indentation>
    class tree_impl
    {
    public:
      explicit tree_impl(std::ostream & stream, std::string_view name) noexcept
          : stream_(stream), name_(name), tag_(stream, name)
      {
      }

      constexpr ~tree_impl() noexcept = default;

      constexpr tree_impl(tree_impl && t) noexcept = default;

      template <typename T>
      tree_impl && operator<<(attr<T> const & a) && noexcept
      {
        tag_ << a;
        return std::move(*this);
      }

      auto operator<<(tag const & t) & noexcept { return tag_ << t; }

      auto operator<<(tree const & t) & noexcept { return tag_ << t; }

    private:
      std::ostream & stream_;
      std::string_view name_;
      tag_impl<indentation, true> tag_; // it is a special tag in the way that it closes on a new line
    };

    struct config
    {
      std::string encoding{"UTF-8"};
      bool standalone{true};
    };

    template <class indentation>
    struct prolog_impl
    {
      explicit prolog_impl(std::ostream & stream, config const & conf) noexcept
      {
        tag_impl<indentation, false, '?'>(stream, "?xml")
            << attr("version", "1.0") << attr("encoding", conf.encoding) << attr("standalone", (conf.standalone ? "yes" : "no"));
      }

      constexpr prolog_impl(prolog_impl &&) noexcept {}
    };

    template <char indent_char = ' ', unsigned indent_increment = 2>
    class doc_impl
    {
    public:
      explicit doc_impl(std::ostream & stream, std::string_view root_name, config const & conf={}) noexcept
          : prolog_(stream, conf), root_(stream, root_name)
      {
      }

      constexpr doc_impl(doc_impl && d) noexcept : prolog_(std::move(d.prolog_)), root_(std::move(d.root_)) {}

      template <typename T>
      doc_impl && operator<<(attr<T> const & a) && noexcept
      {
        std::move(root_) << a;
        return std::move(*this);
      }

      auto operator<<(tag const & t) & noexcept { return root_ << t; }

      auto operator<<(tree const & t) & noexcept { return root_ << t; }

    private:
      prolog_impl<indentation<indent_char, indent_increment, 0>> prolog_;
      tree_impl<indentation<indent_char, indent_increment, 0>> root_;
    };
  } // namespace details

  template <char indent_char = ' ', unsigned indent_inc = 2>
  using doc = details::doc_impl<indent_char, indent_inc>;

  using config = details::config;
} // namespace io1::xml