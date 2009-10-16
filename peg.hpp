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

  template<class Constraint>
  struct chr {
    char v;
    chr(char vv = 0) : v(vv) {}
    operator char() const { return v; }
  };

  template<char s, char e>
  struct chr_range_constraint {
    bool operator()(char c) { return s <= c && c <= e; }
  };

  template<class T, class U>
  struct chr_or_constraint {
    bool operator()(char c) { return T()(c) || U()(c); }
  };

  template<char c>
  struct ch {
    typedef chr<chr_range_constraint<c, c> > type;
  };
  
  template<class T>
  struct constraint_of;

  template<class T>
  struct constraint_of<chr<T> > {
    typedef T type;
  };
  
  typedef chr<chr_range_constraint<'a', 'z'> > lower;
  typedef chr<chr_range_constraint<'A', 'Z'> > upper;
  typedef chr<chr_range_constraint<'0', '9'> > digit;
  typedef chr<chr_or_constraint<constraint_of<lower>::type, constraint_of<upper>::type > > alpha;
  typedef ch<' '>::type ws;

  struct eoi {
  };

  template<class T>
  struct container_for {
    typedef std::vector<T> type;
  };

  template<class T>
  struct container_for<chr<T> > {
    typedef std::string type;
  };
  
  template<class T, class Container = typename container_for<T>::type >
  struct rep : public Container {
  };

  template<class T, class Container = typename container_for<T>::type >
  struct replus : public Container {
  };

  template<class T>
  struct ptr : public T::result_type {
    typedef typename T::result_type base_type;
    ptr() : base_type() {}
    ptr(const base_type& v) : base_type(v) {}
    ptr(const ptr& v) : base_type(v) {}
  };

  template<class T>
  struct opt : public boost::shared_ptr<T> {
  };

  struct sor {
    bool left;
  };

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

  template<class T>
  struct term : public T {
  };

  namespace pi {
    template<class T, class Context>
    struct match_non_term {
      static bool match(T& t, Context *ctx) {
        return parse_fun(t, ctx);
      }
    };

    template<class T, class Context>
    struct match_elem : match_non_term<T, Context> {
    };

    template<class T, class Context>
    struct match_elem<chr<T>, Context> {
      static bool match(chr<T>& t, Context *ctx) {
        if (ctx->cur != ctx->end) {
          char c = *ctx->cur;
          if (T()(c)) {
            t.v = c;
            ++ctx->cur;
            return true;
          }
        }
        return false;
      }
    };
    
    template<class Context>
    struct match_elem<eoi, Context> {
      static bool match(eoi& t, Context *ctx) {
        return ctx->cur == ctx->end;
      }
    };

    template<class T, class C, class Context>
    struct match_elem<rep<T, C>, Context> {
      static bool match(rep<T>& t, Context *ctx) {
        for (;;) {
          T s;
          if (!match_elem<T, Context>::match(s, ctx)) break;
          t.push_back(s);
        }
        return true;
      }
    };

    template<class T, class C, class Context>
    struct match_elem<replus<T, C>, Context> {
      static bool match(replus<T>& t, Context *ctx) {
        {
          T s;
          if (!match_elem<T, Context>::match(s, ctx)) return false;
          t.push_back(s);
        }
        for (;;) {
          T s;
          if (!match_elem<T, Context>::match(s, ctx)) break;
          t.push_back(s);
        }
        return true;
      }
    };
    
    template<class T, class Context>
    struct match_elem<opt<T>, Context> {
      static bool match(opt<T>& t, Context *ctx) {
        T s;
        if (!match_elem<T, Context>::match(s, ctx)) {
          t = 0;
        } else {
          t = new T(s);
        }
        return true;
      }
    };

    template<class Context>
    struct parse_fun_state {
      bool finished;
      bool skipping;
      typename Context::input_type start;
    };

    template<class T, class Context>
    struct rule_step {
      static void run(T& t, parse_fun_state<Context> *state, Context *ctx) {
        if (state->finished || state->skipping) {
        } else {
          state->skipping = !match_elem<T, Context>::match(t, ctx);
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
    struct rule_step<sor, Context> {
      static void run(sor& t, parse_fun_state<Context> *state, Context *ctx) {
        if (state->finished) {
          t.left = true;
        } else if (state->skipping) {
          state->skipping = false;
          ctx->cur = state->start;
          t.left = false;
        } else {
          state->finished = true;
          t.left = true;
        }
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
      state.finished = false;
      state.skipping = false;
      state.start = ctx->cur;
      run_steps_fun<Context> step(&state, ctx);
      boost::fusion::for_each(t, step);
      if (!state.finished && state.skipping) {
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

    template<class T, class Context>
    struct rep_action {
      template<std::size_t n>
      struct dummy_type;

      template<class U>
      static bool call_action(U& t, Context *ctx, dummy_type<0*sizeof(&U::action)>*) {
        return execute_action(&T::action, t, ctx);
      }

      static bool call_action(T& t, Context *ctx, ...) {
        return call_action_0(t, ctx, false, (dummy_type<0>*)0);
      }

#define PEG_CALL_ACTION_N(z, n, d) \
      template<class U> \
        static bool call_action_##n(U& t, Context *ctx, bool executed, dummy_type<0*sizeof(&U::action_##n)>*) { \
        return execute_action(&T::action_##n, t, ctx) || BOOST_PP_CAT(call_action_, BOOST_PP_INC(n))(t, ctx, true, (dummy_type<0>*)0); \
      } \
      \
      static bool call_action_##n(T& t, Context *ctx, bool executed, ...) { \
        return BOOST_PP_CAT(call_action_, BOOST_PP_INC(n))(t, ctx, executed, (dummy_type<0>*)0); \
      }
      
      BOOST_PP_REPEAT(9, PEG_CALL_ACTION_N, 0)

#undef PEG_CALL_ACTION_N
      
      template<class U>
      static bool call_action_9(U& t, Context *ctx, bool executed, dummy_type<0*sizeof(&U::action_9)>*) {
        return execute_action(&T::action_9, t, ctx);
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
      template<class Fun, class Context>
      static bool parse_fun_by_action(Fun fun, ptr<T>& t, Context *ctx) {
        typedef typename boost::function_types::parameter_types<Fun>::type param_list;
        typedef parse_fun_types<param_list> types;
        typename types::result_list_type result;
        if (parse_fun_core(result, ctx)) {
          t = boost::fusion::invoke(fun, result);
          return true;
        } else {
          return false;
        }
      }

      template<class Context>
      static bool parse_fun(ptr<T>& t, Context *ctx) {
        return parse_fun_by_action(&T::action, t, ctx);
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

