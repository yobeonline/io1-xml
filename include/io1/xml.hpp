#pragma once

#include <array>
#include <type_traits>
#include <string_view>
#include <iostream>
#include <cassert>
#include <utility>

namespace io1::xml
{
  namespace details
  {
    constexpr bool is_valid_xml_name_char(char c)
    {
      return true; // std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.' || c == ':';
    }

    constexpr bool is_valid_xml_name_start(char c)
    {
      return true; // std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == ':';
    }

    template <typename Iter>
    constexpr bool validate_xml_name_range(Iter begin, Iter end) noexcept
    {
//      if (begin == end) return false;
//      if (!is_valid_xml_name_start(*begin)) return false;
//      ++begin;
//      while (begin != end)
//      {
//        if (!is_valid_xml_name_char(*begin)) return false;
//        ++begin;
//      }
      return true;
    }

    template <std::size_t N>
    constexpr bool is_ascii_valid_xml_name(const char (&lit)[N]) noexcept
    {
      return validate_xml_name_range(lit, lit + N - 1); // exclude null terminator
    }

    // For runtime use
    inline bool is_ascii_valid_xml_name(std::string_view name) noexcept
    {
      return validate_xml_name_range(name.begin(), name.end());
    }
  } // namespace details

  template <typename T>
  struct attr
  {
    explicit constexpr attr(std::string_view name, T const &value) noexcept:name(name), value(value) {}
    std::string_view name;
    T const &value;
  };

  struct tag
  {
    explicit tag(std::string_view name):name(name)
    {
      if (!details::is_ascii_valid_xml_name(name))
      {
        throw std::invalid_argument("Invalid XML tag name (must be ASCII-valid and XML-safe)");
      }
    }

    template <std::size_t N>
    explicit constexpr tag(const char (&lit)[N]) noexcept : name(lit, N - 1) // exclude null terminator
    {
      static_assert(details::is_ascii_valid_xml_name(lit),
                    "Invalid XML tag name (must be ASCII-valid and XML-safe)");
    }

    std::string_view name;
  };

  struct tree
  {
    constexpr explicit tree(std::string_view name) noexcept:name(name) {}
    std::string_view name;
  };

  namespace details
  {
    template <char indent_char, unsigned indent_increment, unsigned indent_size>
    struct indentation
    {
      using increased_t = indentation<indent_char, indent_increment, indent_size+indent_increment>;

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

    template<typename T> struct is_string
    {
      constexpr static bool value=false;
    };

    template <>
    struct is_string<std::string>
    {
      constexpr static bool value = true;
    };

    template<> struct is_string<std::string_view>
    {
      constexpr static bool value = true;
    };

    template<> 
    struct is_string<char const *>
    {
      constexpr static bool value = true;
    };

    template<> struct is_string<char *>
    {
      constexpr static bool value = true;
    };

    template<typename T> constexpr static bool is_string_v = is_string<T>::value;

    struct done
    {
    };

    template <class indentation>
    class tree_impl;
      
    template <class indentation, bool tree_tag = false, char closing = '/'>
    class tag_impl: private indentation
    {
    public:
      explicit tag_impl(std::ostream & stream, std::string_view name) noexcept:
      stream_(stream), name_(name)
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
        if (empty_)
        {
          stream_ << ' ' << closing << ">\n";
        }
        else
        {
          if constexpr (tree_tag)
          {
            stream_ << indentation::indent() << '<' << closing << name_ << ">\n";
          }
          else
          {
            stream_ << '<' << closing << name_ << ">\n";
          }
        }
      }

      template<typename T> tag_impl & operator<<(attr<T> const & a) noexcept
      {
        stream_ << ' ' << a.name << "=\"" << a.value << '\"';
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
        stream_ << '>';
        if constexpr (is_string_v<T>) write_text(value);
        else stream_ << value;
        empty_ = false;

        return {};
      }

    private:
      void write_text(std::string_view value) noexcept
      {
        for (char c : value)
        {
          switch (c)
          {
          case '&': stream_ << "&amp;"; break;
          case '<': stream_ << "&lt;"; break;
          case '>': stream_ << "&gt;"; break;
          default: stream_ << c; break;
          }
        }
        return;
      }

    private:
      bool active_{true};
      bool empty_{true};
      std::ostream & stream_;
      std::string_view name_;
    };

    template<class indentation>
    class tree_impl
    {
    public:
      explicit tree_impl(std::ostream & stream, std::string_view name) noexcept:
      stream_(stream), name_(name), tag_(stream, name)
      {}

      constexpr ~tree_impl() noexcept =default;

      constexpr tree_impl(tree_impl && t) noexcept =default;

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

    private:
      std::ostream & stream_;
      std::string_view name_;
      tag_impl<indentation, true> tag_; // it is a special tag in the way that it closes on a new line
    };

    template <class indentation>
    struct prolog_impl
    {
      explicit prolog_impl(std::ostream & stream, std::string_view encoding, bool standalone) noexcept
      {
        tag_impl<indentation, false, '?'>(stream, "?xml")
          << attr("version", "1.0")
          << attr("encoding", encoding)
          << attr("standalone", (standalone ? "yes" : "no"));
      }

      constexpr prolog_impl(prolog_impl &&) noexcept {}
    };

    template<char indent_char=' ', unsigned indent_increment=2>
    class doc_impl
    {
    public:
      explicit doc_impl(std::ostream & stream,
        std::string_view root_name,
        std::string_view encoding = "UTF-8",
        bool standalone=true
      ) noexcept:
      prolog_(stream, encoding, standalone),
      root_(stream, root_name)
      {}

      constexpr doc_impl(doc_impl && d) noexcept:
        prolog_(std::move(d.prolog_)),
        root_(std::move(d.root_))
      {}

      template<typename T> doc_impl && operator<<(attr<T> const & a) && noexcept
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

    private:
      prolog_impl<indentation<indent_char, indent_increment, 0>> prolog_;
      tree_impl<indentation<indent_char, indent_increment, 0>> root_;
    };
  }

  template<char indent_char=' ', unsigned indent_inc=2> using doc = details::doc_impl<indent_char,indent_inc>;
}