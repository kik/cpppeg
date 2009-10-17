#ifndef PEG_HPP
#define PEG_HPP

#include <string>
#include <vector>

#if !defined(BOOST_FUSION_INVOKE_MAX_ARITY)
#define BOOST_FUSION_INVOKE_MAX_ARITY 32
#endif

#include <boost/type.hpp>
#include <boost/preprocessor.hpp>
#include <boost/type_traits.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/transform.hpp>

#include <boost/fusion/sequence.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/view.hpp>
#include <boost/fusion/algorithm.hpp>
#include <boost/fusion/functional.hpp>

#include <boost/fusion/adapted/mpl.hpp>

#include <boost/fusion/container/vector/convert.hpp>

namespace peg {
  struct tag_terminal;
  struct tag_non_terminal;

  template<class T>
  struct terminal_trait_base {
    typedef tag_terminal tag_type;
    typedef T value_type;
  };

  struct char_terminal_trait_base : public terminal_trait_base<char> {
  };

  template<class T>
  struct non_terminal_trait_base {
    typedef tag_non_terminal tag_type;
    typedef T value_type;
  };
  
  template<class T>
  struct symbol_trait : public non_terminal_trait_base<T> {
    // default is non-terminal.
  };

  struct char_symbol_base {
    char v;
    char_symbol_base(char vv = 0) : v(vv) {}
    char operator=(char vv) { v = vv; return vv; }
    operator char() const { return v; }
  };

  // matches single char C.
  template<char C>
  struct ch : public char_symbol_base {
    ch(char vv = 0) : char_symbol_base(vv) {}

    template<class Context>
    bool match(Context *ctx) {
      if (ctx->cur != ctx->end && *ctx->cur == C) {
        v = *ctx->cur;
        ++ctx->cur;
        return true;
      }
      return false;
    }
  };
  template<char C>struct symbol_trait<ch<C> > : public char_terminal_trait_base {};

  // matches character range CL <= c <= CH.
  template<char CL, char CH>
  struct chr : public char_symbol_base {
    chr(char vv = 0) : char_symbol_base(vv) {}

    template<class Context>
    bool match(Context *ctx) {
      if (ctx->cur != ctx->end) {
        char c = *ctx->cur;
        if (CL <= c && c <= CH) {
          v = c;
          ++ctx->cur;
          return true;
        }
      }
      return false;
    }
  };
  template<char CL, char CH>struct symbol_trait<chr<CL, CH> > : public char_terminal_trait_base {};

  // matches character terminal T or U.
  template<class T, class U>
  struct cho : public char_symbol_base {
    cho(char vv = 0) : char_symbol_base(vv) {}

    template<class Context>
    bool match(Context *ctx) {
      T t;
      if (t.match(ctx)) {
        v = static_cast<char>(t);
        return true;
      } else {
        U u;
        if (u.match(ctx)) {
          v = static_cast<char>(t);
          return true;
        }
      }
      return false;
    }
  };
  template<class T, class U>struct symbol_trait<cho<T, U> > : public char_terminal_trait_base {};

  typedef chr<'a', 'z'> lower;
  typedef chr<'A', 'Z'> upper;
  typedef chr<'0', '9'> digit;
  typedef cho<lower, upper> alpha;
  typedef cho<ch<' '>, cho<ch<'\n'>, cho<ch<'\r'>, cho<ch<'\t'>, ch<'\f'> > > > > ws;

  // matches end of input.
  struct eoi {
    template<class Context>
    bool match(Context *ctx) {
      return ctx->cur == ctx->end;
    }
  };
  template<>struct symbol_trait<eoi> : public terminal_trait_base<eoi> {};

  namespace pi {
    struct null_struct;
  }

  template<class T = pi::null_struct>
  struct context {
    size_t size;
    size_t pos;
    T *v;
    T *operator->() { return v; }
  };

  namespace pi {
    template<class T, class Tag, class Context>
    struct match_elem_select;

    template<class T, class Context>
    struct match_elem_select<T, tag_terminal, Context> {
      static bool match(T& t, Context *ctx) {
        return t.match(ctx);
      }
    };

    template<class T, class Context>
    struct match_elem_select<T, tag_non_terminal, Context> {
      static bool match(T& t, Context *ctx) {
        return parse_fun(t, ctx);
      }
    };

    template<class T, class Context>
    inline bool match_elem(T& t, Context *ctx) {
      return match_elem_select<T, typename symbol_trait<T>::tag_type, Context>::match(t, ctx);
    }    
  }

  template<class T>
  struct container_for_value {
    typedef std::vector<T> type;
  };

  template<>
  struct container_for_value<char> {
    typedef std::string type;
  };
  
  template<class T>
  struct container_for {
    typedef typename container_for_value<typename symbol_trait<T>::value_type>::type type;
  };
  
  template<class T, class Container = typename container_for<T>::type >
  struct rep : public Container {
    template<class Context>
    bool match(Context *ctx) {
      for (;;) {
        T s;
        if (!pi::match_elem(s, ctx)) break;
        push_back(s);
      }
      return true;
    }
  };
  template<class T, class C>struct symbol_trait<rep<T, C> > : public terminal_trait_base<rep<T> > {};

  template<class T, class Container = typename container_for<T>::type >
  struct replus : public Container {
    template<class Context>
    bool match(Context *ctx) {
      {
        T s;
        if (!pi::match_elem(s, ctx)) return false;
        push_back(s);
      }
      for (;;) {
        T s;
        if (!pi::match_elem(s, ctx)) break;
        push_back(s);
      }
      return true;
    }
  };
  template<class T, class C>struct symbol_trait<replus<T, C> > : public terminal_trait_base<replus<T> > {};

  namespace pi {
    template<class T, bool opp>
    struct if_unless {
      template<class Context>
      bool match(Context *ctx) {
        T s;
        typename Context::input_type cur = ctx->cur;
        if (pi::match_elem(s, ctx)) {
          ctx->cur = cur;
          return true ^ opp;
        }
        return false ^ opp;
      }
    };
  }
  
  template<class T>
  struct if_ : public pi::if_unless<T, false> {
  };
  template<class T>struct symbol_trait<if_<T> > : public terminal_trait_base<if_<T> > {};

  template<class T>
  struct unless : public pi::if_unless<T, true> {
  };
  template<class T>struct symbol_trait<unless<T> > : public terminal_trait_base<unless<T> > {};
  
  template<class T>
  struct ptr : public T::result_type {
    typedef typename T::result_type base_type;
    ptr() : base_type() {}
    ptr(const base_type& v) : base_type(v) {}
    ptr(const ptr& v) : base_type(v) {}
  };

  template<class T>
  struct opt : public boost::shared_ptr<T> {
    typedef boost::shared_ptr<T> base_type;
    template<class Context>
    bool match(Context *ctx) {
      T s;
      if (!pi::match_elem(s, ctx)) {
        base_type::reset();
      } else {
        reset(new T(s));
      }
      return true;
    }
  };
  template<class T>struct symbol_trait<opt<T> > : public terminal_trait_base<opt<T> > {};
  
  namespace pi {
    template<class Context>
    struct parse_fun_state {
      bool skipping;
    };

    template<class T, class Context>
    struct rule_step {
      static void run(T& t, parse_fun_state<Context> *state, Context *ctx) {
        if (state->skipping) {
        } else {
          state->skipping = !match_elem(t, ctx);
        }
      }
    };

    template<class T, class Context>
    struct rule_step<context<T>, Context> {
      static void run(context<T>& t, parse_fun_state<Context> *state, Context *ctx) {
        t.size = ctx->end - ctx->begin;
        t.pos = ctx->cur - ctx->begin;
        t.v = &ctx->t;
      }
    };
    
    template<class Context>
    struct run_steps_fun {
      parse_fun_state<Context> *state;
      Context *context;

      run_steps_fun(parse_fun_state<Context> *st, Context *ctx) : state(st), context(ctx) {
      }
      
      template<class T>
      void operator()(T& t) const {
        rule_step<T, Context>::run(t, state, context);
      }
    };

    template<class T_param_list>
    struct parse_fun_types {
      typedef T_param_list src_param_list_type;
      typedef typename boost::mpl::transform<src_param_list_type,
        boost::remove_const<boost::remove_reference<boost::mpl::_1> > >::type param_list_type;
      typedef typename boost::fusion::result_of::as_vector<param_list_type>::type result_list_type;
    };

    template<class T, class Context>
    inline bool parse_fun_core(T& t, Context *ctx) {
      parse_fun_state<Context> state;
      state.skipping = false;
      run_steps_fun<Context> step(&state, ctx);
      boost::fusion::for_each(t, step);
      if (state.skipping) {
        return false;
      } else {
        return true;
      }
    }
    
    struct parse_action_tag {};
    struct parse_ptr_action_tag {};
    struct parse_sequence_tag {};

    template<class T, class Tag>
    struct select_parse_fun;

    template<class Fun, class T, class Context>
    inline bool execute_action(Fun fun, T& t, Context *ctx) {
      typedef typename boost::function_types::parameter_types<Fun>::type param_list_1;
      typedef typename boost::mpl::pop_front<param_list_1>::type param_list;
      typedef parse_fun_types<param_list> types;
      typename types::result_list_type result;

      typename Context::input_type cur = ctx->cur;
      if (parse_fun_core(result, ctx)) {
        boost::fusion::invoke(fun, boost::fusion::push_front(result, &t));
        return true;
      } else {
        ctx->cur = cur;
        return false;
      }
    }

    template<class Fun, class T, class Context>
    inline bool execute_action(Fun fun, ptr<T>& t, Context *ctx) {
      typedef typename boost::function_types::parameter_types<Fun>::type param_list;
      typedef parse_fun_types<param_list> types;
      typename types::result_list_type result;

      typename Context::input_type cur = ctx->cur;
      if (parse_fun_core(result, ctx)) {
        t = boost::fusion::invoke(fun, result);
        return true;
      } else {
        ctx->cur = cur;
        return false;
      }
    }
    
    template<class T>
    struct get_action_container {
      typedef T type;
    };

    template<class T>
    struct get_action_container<ptr<T> > {
      typedef T type;
    };

    template<class T, class Context>
    struct rep_action {
      template<std::size_t n>
      struct dummy_type;

      template<class U>
      static bool call_action(U& t, Context *ctx, dummy_type<0*sizeof(&get_action_container<U>::type::action)>*) {
        return execute_action(&get_action_container<T>::type::action, t, ctx);
      }

      static bool call_action(T& t, Context *ctx, ...) {
        return call_action_0(t, ctx, false, (dummy_type<0>*)0);
      }

#define PEG_CALL_ACTION_N(z, n, d) \
      template<class U> \
      static bool call_action_##n(U& t, Context *ctx, bool executed, dummy_type<0*sizeof(&get_action_container<U>::type::action_##n)>*) { \
        return execute_action(&get_action_container<T>::type::action_##n, t, ctx) || BOOST_PP_CAT(call_action_, BOOST_PP_INC(n))(t, ctx, true, (dummy_type<0>*)0); \
      } \
      \
      static bool call_action_##n(T& t, Context *ctx, bool executed, ...) { \
        return BOOST_PP_CAT(call_action_, BOOST_PP_INC(n))(t, ctx, executed, (dummy_type<0>*)0); \
      }
      
      BOOST_PP_REPEAT(9, PEG_CALL_ACTION_N, 0)

#undef PEG_CALL_ACTION_N
      
      template<class U>
      static bool call_action_9(U& t, Context *ctx, bool executed, dummy_type<0*sizeof(&get_action_container<U>::type::action_9)>*) {
        return execute_action(&get_action_container<T>::type::action_9, t, ctx);
      }

      static bool call_action_9(T& t, Context *ctx, bool executed, ...) {
        return false;
      }

      static bool start(T& t, Context *ctx) {
        return call_action(t, ctx, (dummy_type<0>*)0);
      }
    };
    
    template<class T>
    struct select_parse_fun<T, parse_action_tag> {
      template<class Context>
      static bool parse_fun(T& t, Context *ctx) {
        return rep_action<T, Context>::start(t, ctx);
      }
    };

    template<class T>
    struct select_parse_fun<ptr<T>, parse_ptr_action_tag> {
      template<class Context>
      static bool parse_fun(ptr<T>& t, Context *ctx) {
        return rep_action<ptr<T>, Context>::start(t, ctx);
      }
    };

    template<class T>
    struct select_parse_fun<T, parse_sequence_tag> {
      template<class Context>
      static bool parse_fun(T& t, Context *ctx) {
        return parse_fun_core(t, ctx);
      }
    };

    template<class T>
    struct parse_tag_of {
      typedef typename boost::mpl::if_<typename boost::fusion::traits::is_sequence<T>::type,
                                       parse_sequence_tag, parse_action_tag>::type type;
    };

    template<class T>
    struct parse_tag_of<ptr<T> > {
      typedef parse_ptr_action_tag type;
    };

    template<class T, class Context>
    inline bool parse_fun(T& t, Context *ctx) {
      return select_parse_fun<T, typename parse_tag_of<T>::type>::parse_fun(t, ctx);
    }

    struct null_struct {};
    
    template<class Input, class T = null_struct>
    struct parser_context {
      T t;
      typedef Input input_type;
      typedef T context_type;
      Input begin;
      Input cur;
      Input end;
    };
  }

  template<class T, class Context = pi::null_struct>
  struct parser {
    parser() {
    }

    template<class Input>
    bool parse(T& t, Input begin, Input end) {
      pi::parser_context<Input, Context> ctx;
      ctx.begin = begin;
      ctx.cur = begin;
      ctx.end = end;
      return pi::parse_fun(t, &ctx);
    }
  };
}
#endif

