
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <Eigen/Dense>
#include "drakeGradientUtil.h"

namespace Drake {

  /** InputOutputRelation
   * \brief Tags which can be used to inform algorithms about underlying structure in a function
   * e.g., linear, affine, polynomial, analytic, differentiable, continuous, measurable, and -- lastly -- arbitrary
   *
   * note: i considered using inheritance to capture the relationship, but passing around types at runtime
   * was more of a pain than simply capturing the inheritance with the helper functions below.
   */
  struct InputOutputRelation {
    enum Form {
      ARBITRARY,
      DIFFERENTIABLE,
      POLYNOMIAL,
      AFFINE,
      LINEAR,
      CONSTANT,
      ZERO
      // todo: add more (rational, ...)
    };
    Form form;
    // todo: add sparsity info

    InputOutputRelation(Form f) : form(f) { };

    static bool isa(const Form &f, const Form &base) {
      if (f == base || base == ARBITRARY) return true;
      if (f == ARBITRARY) return false;
      return isa(derivesFrom(f), base);
    }

    bool isa(Form base) { return isa(form, base); }

    static Form leastCommonAncestor(const Form &f1, const Form &f2) {
      if (f1 == ARBITRARY || f2 == ARBITRARY) return ARBITRARY;
      if (isa(f2, f1)) return f1;
      return leastCommonAncestor(derivesFrom(f1), f2);
    }

    static Form leastCommonAncestor(std::initializer_list<Form> forms) {
      if (forms.size() < 1) throw std::runtime_error("leastCommonAncestor requires at least one argument");
      if (forms.size() == 1) return *forms.begin();

      std::initializer_list<Form>::const_iterator iter = forms.begin();
      Form ret = *iter;
      while (++iter != forms.end()) {
        ret = leastCommonAncestor(ret, *iter);
      }
      return ret;
    }

    static InputOutputRelation composeWith(const InputOutputRelation &g,
                                           const InputOutputRelation &f) { // composition of functions y = g(f(x))
      return InputOutputRelation(leastCommonAncestor(g.form, f.form));
    }

    /*
    static InputOutputRelation linearCombination(const InputOutputRelation& g, const InputOutputRelation& f) {
      return InputOutputRelation(leastCommonAncestor({g.form,f.form,LINEAR}));
    }*/
    static InputOutputRelation combine(const InputOutputRelation &a,
                                       const InputOutputRelation &b) { // vertical concatenation
      return InputOutputRelation(leastCommonAncestor(a.form, b.form));
    }

    static InputOutputRelation combine(std::initializer_list<InputOutputRelation> args) {
      if (args.size() < 1) throw std::runtime_error("combine requires at least one argument");
      if (args.size() == 1) return *args.begin();

      std::initializer_list<InputOutputRelation>::const_iterator iter = args.begin();
      InputOutputRelation ret = *iter;
      while (++iter != args.end()) {
        ret = combine(ret, *iter);
      }
      return ret;
    }

  private:
    static Form derivesFrom(const Form &f) { // capture the inheritance relationships (without resorting to using types)
      switch (f) {
        case DIFFERENTIABLE :
          return ARBITRARY;
        case POLYNOMIAL:
          return DIFFERENTIABLE;
        case AFFINE:
          return POLYNOMIAL;
        case LINEAR:
          return AFFINE;
        case CONSTANT:
          return AFFINE;
        case ZERO:
          return LINEAR; // note: really want multiple inheritance here, since it's also constant
        default:
          return ARBITRARY;
      }
    }
  };

  class Function {
  public:
    Function() : relation(InputOutputRelation::ARBITRARY) {};
    Function(InputOutputRelation::Form f) : relation(f) {};
    virtual ~Function() {};

    virtual void eval(const Eigen::Ref<const Eigen::VectorXd>& x, Eigen::Ref<const Eigen::VectorXd>& y) const = 0;
    virtual const InputOutputRelation& getInputOutputRelation() const { return relation; };

  protected:
    InputOutputRelation relation;
  };

  class DifferentiableFunction : public Function {
  public:
    DifferentiableFunction() : Function(InputOutputRelation::DIFFERENTIABLE) {};
    virtual ~DifferentiableFunction() {};

    virtual void eval(const Eigen::Ref<const TaylorVecXd>& x, Eigen::Ref<const TaylorVecXd>& y) const = 0;
  };

  template <typename Derived>
  class TemplatedDifferentiableFunction : public DifferentiableFunction {
  public:
    TemplatedDifferentiableFunction(Derived& derived) : DifferentiableFunction(), derived(derived) {};
    virtual ~TemplatedDifferentiableFunction() {};

    virtual void eval(const Eigen::Ref<const Eigen::VectorXd>& x, Eigen::Ref<const Eigen::VectorXd>& y) const override { derived.eval(x,y); }
    virtual void eval(const Eigen::Ref<const TaylorVecXd>& x, Eigen::Ref<const TaylorVecXd>& y) const override { derived.eval(x,y); }

  private:
    Derived& derived;
  };

  // idea: use templates to support multi-input, multi-output functions which implement, e.g.
  // void eval(x1,...,xn,  y1,...,ym), and
  // InputOutputRelation getInputOutputRelation(input_index,output_index)

  // idea: avoid dynamic allocation by having derived classes which template on input/output size?
  // hopefully the use of Ref above will mean they can still derive from Function (and avoid the allocation)
};