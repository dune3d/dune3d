//-----------------------------------------------------------------------------
// Once we've written our constraint equations in the symbolic algebra system,
// these routines linearize them, and solve by a modified Newton's method.
// This also contains the routines to detect non-convergence or inconsistency,
// and report diagnostics to the user.
//
// Copyright 2008-2013 Jonathan Westhues.
//-----------------------------------------------------------------------------
#include "solvespace.h"
#include <iostream>
#include <list>

#include <Eigen/Core>
#include <Eigen/SparseQR>

// The solver will converge all unknowns to within this tolerance. This must
// always be much less than LENGTH_EPS, and in practice should be much less.
const double System::CONVERGE_TOLERANCE = (LENGTH_EPS / (1e2));

constexpr size_t LikelyPartialCountPerEq = 10;

bool System::WriteJacobian(int tag) {
    // Clear all
    mat.param.clear();
    mat.eq.clear();
    mat.A.sym.setZero();
    mat.B.sym.clear();

    for(Param &p : param) {
        if(p.tag != tag)
            continue;
        mat.param.push_back(p.h);
    }
    mat.n = mat.param.size();

    for(Equation &e : eq) {
        if(e.tag != tag)
            continue;
        mat.eq.push_back(&e);
    }
    mat.m = mat.eq.size();
    mat.A.sym.resize(mat.m, mat.n);
    mat.A.sym.reserve(Eigen::VectorXi::Constant(mat.n, LikelyPartialCountPerEq));

    // Fill the param id to index map
    std::map<uint32_t, int> paramToIndex;
    for(int j = 0; j < mat.n; j++) {
        paramToIndex[mat.param[j].v] = j;
    }

    if(mat.eq.size() >= MAX_UNKNOWNS) {
        return false;
    }
    std::vector<hParam> paramsUsed;
    // In some experimenting, this is almost always the right size.
    // Value is usually between 0 and 20, comes from number of constraints?
    mat.B.sym.reserve(mat.eq.size());
    for(size_t i = 0; i < mat.eq.size(); i++) {
        Equation *e = mat.eq[i];
        if(e->tag != tag)
            continue;
        // Simplify (fold) then deep-copy the current equation.
        Expr *f = e->e->FoldConstants();
        f       = f->DeepCopyWithParamsAsPointers(&param, &(SK.param));

        paramsUsed.clear();
        f->ParamsUsedList(&paramsUsed);

        for(hParam &p : paramsUsed) {
            // Find the index of this parameter
            auto it = paramToIndex.find(p.v);
            if(it == paramToIndex.end())
                continue;
            // this is the parameter index
            const int j = it->second;
            // compute partial derivative of f
            Expr *pd = f->PartialWrt(p);
            pd       = pd->FoldConstants();
            if(pd->IsZeroConst())
                continue;
            mat.A.sym.insert(i, j) = pd;
        }
        paramsUsed.clear();
        mat.B.sym.push_back(f);
    }
    return true;
}

void System::EvalJacobian() {
    using namespace Eigen;
    mat.A.num.setZero();
    mat.A.num.resize(mat.m, mat.n);
    const int size = mat.A.sym.outerSize();

    for(int k = 0; k < size; k++) {
        for(SparseMatrix<Expr *>::InnerIterator it(mat.A.sym, k); it; ++it) {
            double value = it.value()->Eval();
            if(EXACT(value == 0.0))
                continue;
            mat.A.num.insert(it.row(), it.col()) = value;
        }
    }
    mat.A.num.makeCompressed();
}

bool System::IsDragged(hParam p) {
    const auto b = dragged.begin();
    const auto e = dragged.end();
    return e != std::find(b, e, p);
}

Param *System::GetLastParamSubstitution(Param *p) {
    Param *current = p;
    while(current->substd != NULL) {
        current = current->substd;
        if(current == p) {
            // Break the loop
            current->substd = NULL;
            break;
        }
    }
    return current;
}

void System::SortSubstitutionByDragged(Param *p) {
    std::vector<Param *> subsParams;
    Param *by      = NULL;
    Param *current = p;
    while(current != NULL) {
        subsParams.push_back(current);
        if(IsDragged(current->h)) {
            by = current;
        }
        current = current->substd;
    }
    if(by == NULL)
        by = p;
    for(Param *p : subsParams) {
        if(p == by)
            continue;
        p->substd = by;
        p->tag    = VAR_SUBSTITUTED;
    }
    by->substd = NULL;
    by->tag    = 0;
}

void System::SubstituteParamsByLast(Expr *e) {
    ssassert(e->op != Expr::Op::PARAM_PTR,
             "Expected an expression that refer to params via handles");

    if(e->op == Expr::Op::PARAM) {
        Param *p = param.FindByIdNoOops(e->parh);
        if(p != NULL) {
            Param *s = GetLastParamSubstitution(p);
            if(s != NULL) {
                e->parh = s->h;
            }
        }
    } else {
        int c = e->Children();
        if(c >= 1) {
            SubstituteParamsByLast(e->a);
            if(c >= 2)
                SubstituteParamsByLast(e->b);
        }
    }
}

void System::SolveBySubstitution() {
    for(auto &teq : eq) {
        Expr *tex = teq.e;

        if(tex->op == Expr::Op::MINUS && tex->a->op == Expr::Op::PARAM &&
           tex->b->op == Expr::Op::PARAM) {
            hParam a = tex->a->parh;
            hParam b = tex->b->parh;
            if(!(param.FindByIdNoOops(a) && param.FindByIdNoOops(b))) {
                // Don't substitute unless they're both solver params;
                // otherwise it's an equation that can be solved immediately,
                // or an error to flag later.
                continue;
            }

            if(a.v == b.v) {
                teq.tag = EQ_SUBSTITUTED;
                continue;
            }

            Param *pa = param.FindById(a);
            Param *pb = param.FindById(b);

            // Take the last substitution of parameter a
            // This resulted in creation of substitution chains
            Param *last  = GetLastParamSubstitution(pa);
            last->substd = pb;
            last->tag    = VAR_SUBSTITUTED;

            if(pb->substd != NULL) {
                // Break the loops
                GetLastParamSubstitution(pb);
                // if b loop was broken
                if(pb->substd == NULL) {
                    // Clear substitution
                    pb->tag = 0;
                }
            }
            teq.tag = EQ_SUBSTITUTED;
        }
    }

    //
    for(Param &p : param) {
        SortSubstitutionByDragged(&p);
    }

    // Substitute all the equations
    for(auto &req : eq) {
        SubstituteParamsByLast(req.e);
    }

    // Substitute all the parameters with last substitutions
    for(auto &p : param) {
        if(p.substd == NULL)
            continue;
        p.substd = GetLastParamSubstitution(p.substd);
    }
}

//-----------------------------------------------------------------------------
// Calculate the rank of the Jacobian matrix
//-----------------------------------------------------------------------------
int System::CalculateRank() {
    using namespace Eigen;
    if(mat.n == 0 || mat.m == 0)
        return 0;
    SparseQR<SparseMatrix<double>, COLAMDOrdering<int>> solver;
    solver.compute(mat.A.num);
    int result = solver.rank();
    return result;
}

bool System::TestRank(int *dof) {
    EvalJacobian();
    int jacobianRank = CalculateRank();
    // We are calculating dof based on real rank, not mat.m.
    // Using this approach we can calculate real dof even when redundant is allowed.
    if(dof != NULL)
        *dof = mat.n - jacobianRank;
    return jacobianRank == mat.m;
}

bool System::SolveLinearSystem(const Eigen::SparseMatrix<double> &A, const Eigen::VectorXd &B,
                               Eigen::VectorXd *X) {
    if(A.outerSize() == 0)
        return true;
    using namespace Eigen;
    SparseQR<SparseMatrix<double>, COLAMDOrdering<int>> solver;
    // SimplicialLDLT<SparseMatrix<double>> solver;
    solver.compute(A);
    *X = solver.solve(B);
    return (solver.info() == Success);
}

bool System::SolveLeastSquares() {
    using namespace Eigen;
    // Scale the columns; this scale weights the parameters for the least
    // squares solve, so that we can encourage the solver to make bigger
    // changes in some parameters, and smaller in others.
    mat.scale = VectorXd::Ones(mat.n);
    for(int c = 0; c < mat.n; c++) {
        if(IsDragged(mat.param[c])) {
            // It's least squares, so this parameter doesn't need to be all
            // that big to get a large effect.
            mat.scale[c] = 1 / 20.0;
        }
    }

    const int size = mat.A.num.outerSize();
    for(int k = 0; k < size; k++) {
        for(SparseMatrix<double>::InnerIterator it(mat.A.num, k); it; ++it) {
            it.valueRef() *= mat.scale[it.col()];
        }
    }

    SparseMatrix<double> AAt = mat.A.num * mat.A.num.transpose();
    AAt.makeCompressed();
    VectorXd z(mat.n);

    if(!SolveLinearSystem(AAt, mat.B.num, &z))
        return false;

    mat.X = mat.A.num.transpose() * z;

    for(int c = 0; c < mat.n; c++) {
        mat.X[c] *= mat.scale[c];
    }
    return true;
}

bool System::NewtonSolve(int tag) {

    int iter       = 0;
    bool converged = false;
    int i;

    // Evaluate the functions at our operating point.
    mat.B.num = Eigen::VectorXd(mat.m);
    for(i = 0; i < mat.m; i++) {
        mat.B.num[i] = (mat.B.sym[i])->Eval();
    }
    do {
        // And evaluate the Jacobian at our initial operating point.
        EvalJacobian();

        if(!SolveLeastSquares())
            break;

        // Take the Newton step;
        //      J(x_n) (x_{n+1} - x_n) = 0 - F(x_n)
        for(i = 0; i < mat.n; i++) {
            Param *p = param.FindById(mat.param[i]);
            p->val -= mat.X[i];
            if(IsReasonable(p->val)) {
                // Very bad, and clearly not convergent
                return false;
            }
        }

        // Re-evalute the functions, since the params have just changed.
        for(i = 0; i < mat.m; i++) {
            mat.B.num[i] = (mat.B.sym[i])->Eval();
        }
        // Check for convergence
        converged = true;
        for(i = 0; i < mat.m; i++) {
            if(IsReasonable(mat.B.num[i])) {
                return false;
            }
            if(fabs(mat.B.num[i]) > CONVERGE_TOLERANCE) {
                converged = false;
                break;
            }
        }
    } while(iter++ < 50 && !converged);

    return converged;
}

void System::WriteEquationsExceptFor(hConstraint hc, Group *g) {
    // Generate all the equations from constraints in this group
    for(auto &con : SK.constraint) {
        ConstraintBase *c = &con;
        if(c->group != g->h)
            continue;
        if(c->h == hc)
            continue;

        if(c->HasLabel() && c->type != Constraint::Type::COMMENT && g->allDimsReference) {
            // When all dimensions are reference, we adjust them to display
            // the correct value, and then don't generate any equations.
            c->ModifyToSatisfy();
            continue;
        }
        if(g->relaxConstraints && c->type != Constraint::Type::POINTS_COINCIDENT) {
            // When the constraints are relaxed, we keep only the point-
            // coincident constraints, and the constraints generated by
            // the entities and groups.
            continue;
        }

        c->GenerateEquations(&eq);
    }
    // And the equations from entities
    for(auto &ent : SK.entity) {
        EntityBase *e = &ent;
        if(e->group != g->h)
            continue;

        e->GenerateEquations(&eq);
    }
    // And from the groups themselves
    g->GenerateEquations(&eq);
}

void System::FindWhichToRemoveToFixJacobian(Group *g, List<hConstraint> *bad, bool forceDofCheck) {
    auto time         = GetMilliseconds();
    g->solved.timeout = false;
    int a;

    for(a = 0; a < 2; a++) {
        for(auto &con : SK.constraint) {
            if((GetMilliseconds() - time) > g->solved.findToFixTimeout) {
                g->solved.timeout = true;
                return;
            }

            ConstraintBase *c = &con;
            if(c->group != g->h)
                continue;
            if((c->type == Constraint::Type::POINTS_COINCIDENT && a == 0) ||
               (c->type != Constraint::Type::POINTS_COINCIDENT && a == 1)) {
                // Do the constraints in two passes: first everything but
                // the point-coincident constraints, then only those
                // constraints (so they appear last in the list).
                continue;
            }

            param.ClearTags();
            eq.Clear();
            WriteEquationsExceptFor(c->h, g);
            eq.ClearTags();

            // It's a major speedup to solve the easy ones by substitution here,
            // and that doesn't break anything.
            if(!forceDofCheck) {
                SolveBySubstitution();
            }

            WriteJacobian(0);
            EvalJacobian();

            int rank = CalculateRank();
            if(rank == mat.m) {
                // We fixed it by removing this constraint
                bad->Add(&(c->h));
            }
        }
    }
}

struct Rearraged {
    Expr *lhs = nullptr;
    Expr *rhs = nullptr;

    void isolate(hParam param);
};

void Rearraged::isolate(hParam param) {
    // rearrange equation until param is the only thing on lhs
    while(lhs->op != Expr::Op::PARAM_PTR && lhs->op != Expr::Op::PARAM) {
        switch(lhs->op) {
        case Expr::Op::PLUS:
            if(lhs->a->DependsOn(param)) {
                rhs = rhs->Minus(lhs->b);
                lhs = lhs->a;
            } else if(lhs->b->DependsOn(param)) {
                rhs = rhs->Minus(lhs->a);
                lhs = lhs->b;
            } else {
                ssassert(false, "param was not referenced");
            }
            break;
        case Expr::Op::MINUS:
            if(lhs->a->DependsOn(param)) {
                rhs = rhs->Plus(lhs->b);
                lhs = lhs->a;
            } else if(lhs->b->DependsOn(param)) {
                rhs = (rhs->Minus(lhs->a))->Negate();
                lhs = lhs->b;
            } else {
                ssassert(false, "param was not referenced");
            }
            break;
        case Expr::Op::TIMES:
            if(lhs->a->DependsOn(param)) {
                rhs = rhs->Div(lhs->b);
                lhs = lhs->a;
            } else if(lhs->b->DependsOn(param)) {
                rhs = rhs->Div(lhs->a);
                lhs = lhs->b;
            } else {
                ssassert(false, "param was not referenced");
            }
            break;
        case Expr::Op::SQRT:
            lhs = lhs->a;
            rhs = rhs->Square();
            break;
        case Expr::Op::SQUARE:
            lhs = lhs->a;
            rhs = rhs->Sqrt();
            {
                const auto lhs_ev     = lhs->Eval();
                const auto rhs_ev     = rhs->Eval();
                const auto rhs_ev_neg = -rhs_ev;
                if(std::abs(rhs_ev_neg - lhs_ev) < std::abs(lhs_ev - rhs_ev)) {
                    std::cout << "negate " << std::endl;
                    rhs = rhs->Negate();
                }
            }
            break;
        default: ssassert(false, "unsupported op in isolate");
        }
        // std::cout << "re " << lhs->Print() << " = " << rhs->Print() << std::endl;
    }
    if(lhs->op == Expr::Op::PARAM_PTR)
        ssassert(lhs->parp->h == param, "lhs param was not expected param");
    else if(lhs->op == Expr::Op::PARAM)
        ssassert(lhs->parh == param, "lhs param was not expected param");
    else
        ssassert(false, "Unexpected lhs");
}

static void add_expr(Expr *e, Equation *eq,
                     std::map<uint32_t, std::map<Equation *, unsigned int>> &usage,
                     std::map<Equation *, std::set<uint32_t>> &eq_params) {
    if(e->op == Expr::Op::PARAM) {
        eq_params[eq].insert(e->parh.v);
        auto &x = usage[e->parh.v];
        if(x.contains(eq))
            x.at(eq)++;
        else
            x.emplace(eq, 1);
        return;
    }
    ssassert(e->op != Expr::Op::PARAM_PTR,
             "Expected an expression that refer to params via handles");

    int c = e->Children();
    if(c == 0) {
        return;
    } else if(c == 1) {
        add_expr(e->a, eq, usage, eq_params);
    } else if(c == 2) {
        add_expr(e->a, eq, usage, eq_params);
        add_expr(e->b, eq, usage, eq_params);
    } else
        ssassert(false, "Unexpected children count");
}

SolveResult System::Solve(Group *g, int *rank, int *dof, List<hConstraint> *bad, bool andFindBad,
                          bool andFindFree, bool forceDofCheck) {
    WriteEquationsExceptFor(Constraint::NO_CONSTRAINT, g);

    bool rankOk;


    // All params and equations are assigned to group zero.
    param.ClearTags();
    eq.ClearTags();

    auto tsubst_begin = clock();
    // Since we are suppressing dof calculation or allowing redundant, we
    // can't / don't want to catch result of dof checking without substitution
    if(g->suppressDofCalculation || g->allowRedundant || !forceDofCheck) {
        SolveBySubstitution();
    }
    auto tsubst_end = clock();
    std::cout << "subst took " << (double)(tsubst_end - tsubst_begin) / CLOCKS_PER_SEC << std::endl;


    // Before solving the big system, see if we can find any equations that
    // are soluble alone. This can be a huge speedup. We don't know whether
    // the system is consistent yet, but if it isn't then we'll catch that
    // later.

    unsigned int n_eq0 = 0, n_param0 = 0;
    unsigned int nref1 = 0;
    int tag1;
    int alone = 1;
    std::list<std::map<unsigned int, Equation *>> param_exprs;
    std::map<uint32_t, std::map<Equation *, unsigned int>> param_equation_usage;
    std::map<Equation *, std::set<uint32_t>> equation_params;
    clock_t talone_end, tfind1_end, tsolve_end, tfinish_end;
    for(auto &e : eq) {
        if(e.tag != 0)
            continue;

        hParam hp = e.e->ReferencedParams(&param);
        if(hp == Expr::NO_PARAMS)
            continue;
        if(hp == Expr::MULTIPLE_PARAMS)
            continue;

        Param *p = param.FindById(hp);
        if(p->tag != 0)
            continue; // let rank test catch inconsistency

        e.tag  = alone;
        p->tag = alone;

        if(e.e->CountParamReferences(hp) == 1) {
            Rearraged rearraged;
            rearraged.lhs = e.e;
            rearraged.rhs = Expr::From(0);
            rearraged.isolate(hp);
            p->val   = rearraged.rhs->Eval();
            p->known = true;
        }
        else {
            WriteJacobian(alone);
            if(!NewtonSolve(alone)) {
                // We don't do the rank test, so let's arbitrarily return
                // the DIDNT_CONVERGE result here.
                rankOk = true;
                // Failed to converge, bail out early
                goto didnt_converge;
            }
        }
        alone++;
    }
    talone_end = clock();
    std::cout << "alone took " << (double)(talone_end - tsubst_end) / CLOCKS_PER_SEC << std::endl;
    if(0) {
        int x;

        dbp("%d equations", eq.n);
        for(x = 0; x < eq.n; x++) {
            if(eq[x].tag != 0)
                continue;
            dbp("%x[%d] %.3f = %s = 0", eq[x].h.v, eq[x].tag, eq[x].e->Eval(),
                eq[x].e->Print().c_str());
        }
        dbp("%d parameters", param.n);
        for(x = 0; x < param.n; x++) {
            if(param[x].tag != 0)
                continue;
            dbp("   param %08x[%d] at %.3f", param[x].h.v, param[x].tag, param[x].val);
        }
    }

    for(auto &e : eq) {
        if(e.tag != 0)
            continue;
        add_expr(e.e, &e, param_equation_usage, equation_params);
    }
    tag1 = 1000;
    while(1) {
        std::map<unsigned int, Equation *> param_expr;
        n_eq0    = 0;
        n_param0 = 0;
        for(auto &e : eq) {
            if(e.tag == 0)
                n_eq0++;
        }
        for(auto &p : param) {
            if(p.tag == 0)
                n_param0++;
        }
        //std::cout << "neq0:" << n_eq0 << " nparam0:" << n_param0 << " tag1:" << tag1 << std::endl;

        nref1 = 0;


        for(auto &p : param) {
            if(IsDragged(p.h))
                continue;
            if(p.tag != 0)
                continue;
            unsigned int nrefs = 0;
            Equation *eq1      = nullptr;

            if(param_equation_usage.contains(p.h.v)) {
                auto &eqs = param_equation_usage.at(p.h.v);
                for(auto &[e, n] : eqs) {
                    if(e->tag != 0 && e->tag < tag1)
                        continue;
                    if(n == 1)
                        eq1 = e;
                    nrefs += n;
                }
            }

            if(nrefs == 1) {
                // only remove equation and param from system if expression doesn't contain
                // parameters only referenced by this equation
                auto &params    = equation_params.at(eq1);
                bool can_remove = true;
                for(auto para : params) {
                    if(para == p.h.v) // this parameter, don't care
                        continue;
                    auto &eqs        = param_equation_usage.at(para);
                    unsigned int neq = 0;
                    for(auto &e : eqs) {
                        if(e.first == eq1)
                            continue;
                        if(e.first->tag != 0 && e.first->tag < tag1)
                            continue;
                        neq++;
                    }
                    if(neq == 0) {
                        can_remove = false;
                        break;
                    }
                }

                // std::cout << "ref1 param " << std::hex << p.h.v << std::endl;
                if(can_remove) {
                    p.tag    = tag1;
                    eq1->tag = tag1;
                    nref1++;
                    param_expr.emplace(p.h.v, eq1);
                }
            }
        }
        tag1++;

        if(param_expr.size())
            param_exprs.push_front(param_expr);
        else
            break;

        // break;
    }
    tfind1_end = clock();
    std::cout << "find1 took " << (double)(tfind1_end - talone_end) / CLOCKS_PER_SEC << std::endl;

    // Now write the Jacobian for what's left, and do a rank test; that
    // tells us if the system is inconsistently constrained.
    if(!WriteJacobian(0)) {
        return SolveResult::TOO_MANY_UNKNOWNS;
    }
    // Clear dof value in order to have indication when dof is actually not calculated
    if(dof != NULL)
        *dof = -1;
    // We are suppressing or allowing redundant, so we no need to catch unsolveable + redundant
    rankOk = (!g->suppressDofCalculation && !g->allowRedundant) ? TestRank(dof) : true;

    // And do the leftovers as one big system
    if(!NewtonSolve(0)) {
        goto didnt_converge;
    }

    // Here we are want to calculate dof even when redundant is allowed, so just handle suppressing
    rankOk = (!g->suppressDofCalculation) ? TestRank(dof) : true;
    if(!rankOk) {
        if(andFindBad)
            FindWhichToRemoveToFixJacobian(g, bad, forceDofCheck);
    } else {
        MarkParamsFree(andFindFree);
    }

    tsolve_end = clock();
    std::cout << "solve took " << (double)(tsolve_end - tfind1_end) / CLOCKS_PER_SEC << std::endl;

    
    for(auto &p : param) {
        if(p.tag == VAR_SUBSTITUTED) {
            p.val = p.substd->val;
        }
    }
    

    for(auto &param_expr : param_exprs) {
        for(auto &[p, e] : param_expr) {
            Rearraged rearraged;
            Expr *f       = e->e->FoldConstants();
            f             = f->DeepCopyWithParamsAsPointers(&param, &(SK.param));
            rearraged.lhs = f;
            rearraged.rhs = Expr::From(0);
            // std::cout << std::endl;
            // printf("%08x\n", e->h.v);
            // std::cout << std::hex << p << std::endl;
            // std::cout << rearraged.lhs->Print() << " = " <<rearraged.rhs->Print() << std::endl;
            rearraged.isolate(hParam{p});
            // std::cout << rearraged.lhs->Print() << " = " <<rearraged.rhs->Print() << std::endl;
            Param *pp = param.FindById(hParam{p});
            pp->val   = rearraged.rhs->Eval();
            pp->known = true;
        }
        // std::cout << "---" << std::endl;
    }

    for(auto &p : param) {
        if(p.tag == VAR_SUBSTITUTED) {
            p.val = p.substd->val;
        }
    }

    // System solved correctly, so write the new values back in to the
    // main parameter table.
    for(auto &p : param) {
        double val;

        val = p.val;

        Param *pp = SK.GetParam(p.h);
        pp->val   = val;
        pp->known = true;
        pp->free  = p.free;
    }
    tfinish_end = clock();
    std::cout << "finish took " << (double)(tfinish_end - tsolve_end) / CLOCKS_PER_SEC << std::endl;
    // equations and params with tag=tag1 can be solved in a symbolic way


    if(0) {
        int x;
        dbp("solved %d equations", eq.n);
        for(x = 0; x < eq.n; x++) {
            dbp("%x[%d] %.3f = %s = 0", eq[x].h.v, eq[x].tag, eq[x].e->Eval(),
                eq[x].e->Print().c_str());
        }
        dbp("solved %d parameters", param.n);
        for(x = 0; x < param.n; x++) {
            dbp("   param %08x[%d] at %.3f", param[x].h.v, param[x].tag, param[x].val);
        }
    }

    return rankOk ? SolveResult::OKAY : SolveResult::REDUNDANT_OKAY;

didnt_converge:
    SK.constraint.ClearTags();
    // Not using range-for here because index is used in additional ways
    for(size_t i = 0; i < mat.eq.size(); i++) {
        if(fabs(mat.B.num[i]) > CONVERGE_TOLERANCE || IsReasonable(mat.B.num[i])) {
            // This constraint is unsatisfied.
            if(!mat.eq[i]->h.isFromConstraint())
                continue;

            hConstraint hc    = mat.eq[i]->h.constraint();
            ConstraintBase *c = SK.constraint.FindByIdNoOops(hc);
            if(!c)
                continue;
            // Don't double-show constraints that generated multiple
            // unsatisfied equations
            if(!c->tag) {
                bad->Add(&(c->h));
                c->tag = 1;
            }
        }
    }

    return rankOk ? SolveResult::DIDNT_CONVERGE : SolveResult::REDUNDANT_DIDNT_CONVERGE;
}

SolveResult System::SolveRank(Group *g, int *rank, int *dof, List<hConstraint> *bad,
                              bool andFindBad, bool andFindFree) {
    WriteEquationsExceptFor(Constraint::NO_CONSTRAINT, g);

    // All params and equations are assigned to group zero.
    param.ClearTags();
    eq.ClearTags();

    // Now write the Jacobian, and do a rank test; that
    // tells us if the system is inconsistently constrained.
    if(!WriteJacobian(0)) {
        return SolveResult::TOO_MANY_UNKNOWNS;
    }

    bool rankOk = TestRank(dof);
    if(!rankOk) {
        // When we are testing with redundant allowed, we don't want to have additional info
        // about redundants since this test is working only for single redundant constraint
        if(!g->suppressDofCalculation && !g->allowRedundant) {
            if(andFindBad)
                FindWhichToRemoveToFixJacobian(g, bad, true);
        }
    } else {
        MarkParamsFree(andFindFree);
    }
    return rankOk ? SolveResult::OKAY : SolveResult::REDUNDANT_OKAY;
}

void System::Clear() {
    entity.Clear();
    param.Clear();
    eq.Clear();
    dragged.Clear();
    mat.A.num.setZero();
    mat.A.sym.setZero();
}

void System::MarkParamsFree(bool find) {
    // If requested, find all the free (unbound) variables. This might be
    // more than the number of degrees of freedom. Don't always do this,
    // because the display would get annoying and it's slow.
    for(auto &p : param) {
        p.free = false;

        if(find) {
            if(p.tag == 0) {
                p.tag = VAR_DOF_TEST;
                WriteJacobian(0);
                EvalJacobian();
                int rank = CalculateRank();
                if(rank == mat.m) {
                    p.free = true;
                }
                p.tag = 0;
            }
        }
    }
}

