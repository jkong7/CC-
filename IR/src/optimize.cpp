#include <optimize.h>

#include <map>
#include <set>
#include <functional>

namespace IR {

  struct Slots {
    std::vector<Item**> items;
    std::vector<Variable**> vars;
  };

  static Slots use_slots(Instruction* i) {
    Slots s;
    if (auto* x = dynamic_cast<Instruction_assignment*>(i)) {
      s.items.push_back(&x->src_);
    } else if (auto* x = dynamic_cast<Instruction_op*>(i)) {
      s.items.push_back(&x->lhs_);
      s.items.push_back(&x->rhs_);
    } else if (auto* x = dynamic_cast<Instruction_index_load*>(i)) {
      s.vars.push_back(&x->src_);
      for (auto &idx : x->indexes_) s.items.push_back(&idx);
    } else if (auto* x = dynamic_cast<Instruction_index_store*>(i)) {
      s.vars.push_back(&x->dst_);
      for (auto &idx : x->indexes_) s.items.push_back(&idx);
      s.items.push_back(&x->src_);
    } else if (auto* x = dynamic_cast<Instruction_length_t*>(i)) {
      s.vars.push_back(&x->src_);
      s.items.push_back(&x->t_);
    } else if (auto* x = dynamic_cast<Instruction_length*>(i)) {
      s.vars.push_back(&x->src_);
    } else if (auto* x = dynamic_cast<Instruction_call*>(i)) {
      if (x->callee_) s.items.push_back(&x->callee_);
      for (auto &a : x->args_) s.items.push_back(&a);
    } else if (auto* x = dynamic_cast<Instruction_call_assignment*>(i)) {
      if (x->callee_) s.items.push_back(&x->callee_);
      for (auto &a : x->args_) s.items.push_back(&a);
    } else if (auto* x = dynamic_cast<Instruction_new_array*>(i)) {
      for (auto &a : x->args_) s.items.push_back(&a);
    } else if (auto* x = dynamic_cast<Instruction_new_tuple*>(i)) {
      s.items.push_back(&x->t_);
    } else if (auto* x = dynamic_cast<Instruction_break_cond*>(i)) {
      s.items.push_back(&x->t_);
    } else if (auto* x = dynamic_cast<Instruction_return_t*>(i)) {
      s.items.push_back(&x->t_);
    }
    return s;
  }

  static Variable* defined(Instruction* i) {
    if (auto* x = dynamic_cast<Instruction_assignment*>(i))       return x->dst_;
    if (auto* x = dynamic_cast<Instruction_op*>(i))               return x->dst_;
    if (auto* x = dynamic_cast<Instruction_index_load*>(i))       return x->dst_;
    if (auto* x = dynamic_cast<Instruction_length_t*>(i))         return x->dst_;
    if (auto* x = dynamic_cast<Instruction_length*>(i))           return x->dst_;
    if (auto* x = dynamic_cast<Instruction_call_assignment*>(i))  return x->dst_;
    if (auto* x = dynamic_cast<Instruction_new_array*>(i))        return x->dst_;
    if (auto* x = dynamic_cast<Instruction_new_tuple*>(i))        return x->dst_;
    return nullptr;
  }

  static bool removable(Instruction* i) {
    return dynamic_cast<Instruction_assignment*>(i)
        || dynamic_cast<Instruction_op*>(i)
        || dynamic_cast<Instruction_index_load*>(i)
        || dynamic_cast<Instruction_length_t*>(i)
        || dynamic_cast<Instruction_length*>(i)
        || dynamic_cast<Instruction_new_array*>(i)
        || dynamic_cast<Instruction_new_tuple*>(i);
  }

  static std::set<std::string> used_names(Instruction* i) {
    std::set<std::string> names;
    Slots s = use_slots(i);
    for (auto** item : s.items) {
      if (auto* v = dynamic_cast<Variable*>(*item)) names.insert(v->var_);
    }
    for (auto** v : s.vars) names.insert((*v)->var_);
    return names;
  }

  static Instruction* terminator(BasicBlock* bb) {
    return bb->instructions.empty() ? nullptr : bb->instructions.back();
  }

  static void rebuild_cfg(Function* f) {
    f->label_to_bb.clear();
    for (auto* bb : f->basic_blocks) f->label_to_bb[bb->label_->label_] = bb;
    for (auto* bb : f->basic_blocks) {
      bb->succ_labels.clear();
      bb->succs.clear();
      Instruction* t = terminator(bb);
      if (auto* u = dynamic_cast<Instruction_break_uncond*>(t)) {
        bb->succ_labels.push_back(u->label_->label_);
      } else if (auto* c = dynamic_cast<Instruction_break_cond*>(t)) {
        bb->succ_labels.push_back(c->label1_->label_);
        bb->succ_labels.push_back(c->label2_->label_);
      }
      for (auto &l : bb->succ_labels) bb->succs.push_back(f->label_to_bb.at(l));
    }
  }

  static std::map<BasicBlock*, std::vector<BasicBlock*>> predecessors(Function* f) {
    std::map<BasicBlock*, std::vector<BasicBlock*>> preds;
    for (auto* bb : f->basic_blocks) {
      preds[bb];
      for (auto* s : bb->succs) preds[s].push_back(bb);
    }
    return preds;
  }

  /*
   * Constant propagation.
   */

  struct Value {
    enum Kind { top, number, function, bottom } kind = top;
    int64_t n = 0;
    std::string f;

    bool operator==(const Value &o) const {
      return kind == o.kind && n == o.n && f == o.f;
    }
    bool operator!=(const Value &o) const { return !(*this == o); }
  };

  using Env = std::map<std::string, Value>;

  static Value meet(const Value &a, const Value &b) {
    if (a.kind == Value::top) return b;
    if (b.kind == Value::top) return a;
    if (a == b) return a;
    return Value{Value::bottom};
  }

  static Value lookup(const Env &env, const std::string &name) {
    auto it = env.find(name);
    return it == env.end() ? Value{Value::top} : it->second;
  }

  static Value value_of(const Env &env, Item* item) {
    if (auto* n = dynamic_cast<Number*>(item)) return Value{Value::number, n->number_};
    if (auto* f = dynamic_cast<Func*>(item)) return Value{Value::function, 0, f->function_label_};
    if (auto* v = dynamic_cast<Variable*>(item)) return lookup(env, v->var_);
    return Value{Value::bottom};
  }

  static int64_t fold(OP op, int64_t lhs, int64_t rhs) {
    uint64_t a = static_cast<uint64_t>(lhs);
    uint64_t b = static_cast<uint64_t>(rhs);
    switch (op) {
      case OP::plus:                return static_cast<int64_t>(a + b);
      case OP::minus:               return static_cast<int64_t>(a - b);
      case OP::times:               return static_cast<int64_t>(a * b);
      case OP::at:                  return lhs & rhs;
      case OP::left_shift:          return static_cast<int64_t>(a << (b & 63));
      case OP::right_shift:         return lhs >> (b & 63);
      case OP::less_than:           return lhs < rhs;
      case OP::less_than_equal:     return lhs <= rhs;
      case OP::equal:               return lhs == rhs;
      case OP::greater_than_equal:  return lhs >= rhs;
      case OP::greater_than:        return lhs > rhs;
    }
    return 0;
  }

  static void transfer(Env &env, Instruction* i) {
    Variable* d = defined(i);
    if (!d) return;
    Value v{Value::bottom};
    if (auto* a = dynamic_cast<Instruction_assignment*>(i)) {
      v = value_of(env, a->src_);
    } else if (auto* o = dynamic_cast<Instruction_op*>(i)) {
      Value l = value_of(env, o->lhs_);
      Value r = value_of(env, o->rhs_);
      if (l.kind == Value::number && r.kind == Value::number) {
        v = Value{Value::number, fold(o->op_, l.n, r.n)};
      } else if (l.kind == Value::top || r.kind == Value::top) {
        v = Value{Value::top};
      }
    }
    env[d->var_] = v;
  }

  static Item* constant_item(const Value &v) {
    if (v.kind == Value::number) return new Number(v.n);
    if (v.kind == Value::function) return new Func(v.f);
    return nullptr;
  }

  static bool propagate_constants(Function* f) {
    if (f->basic_blocks.empty()) return false;
    auto preds = predecessors(f);
    std::map<BasicBlock*, Env> out;

    Env entry;
    for (auto* p : f->var_arguments) entry[p->var_] = Value{Value::bottom};

    bool changed = true;
    while (changed) {
      changed = false;
      for (auto* bb : f->basic_blocks) {
        Env env = bb == f->basic_blocks[0] ? entry : Env{};
        for (auto* p : preds[bb]) {
          for (auto &[name, value] : out[p]) env[name] = meet(lookup(env, name), value);
        }
        for (auto* i : bb->instructions) transfer(env, i);
        if (env != out[bb]) {
          out[bb] = env;
          changed = true;
        }
      }
    }

    bool rewrote = false;
    for (auto* bb : f->basic_blocks) {
      Env env = bb == f->basic_blocks[0] ? entry : Env{};
      for (auto* p : preds[bb]) {
        for (auto &[name, value] : out[p]) env[name] = meet(lookup(env, name), value);
      }
      for (auto &i : bb->instructions) {
        Slots s = use_slots(i);
        for (auto** item : s.items) {
          auto* v = dynamic_cast<Variable*>(*item);
          if (!v) continue;
          Item* c = constant_item(lookup(env, v->var_));
          if (!c) continue;
          bool is_callee = false;
          if (auto* call = dynamic_cast<Instruction_call*>(i)) is_callee = item == &call->callee_;
          if (auto* call = dynamic_cast<Instruction_call_assignment*>(i)) is_callee = item == &call->callee_;
          if (dynamic_cast<Func*>(c) && !is_callee && !dynamic_cast<Instruction_assignment*>(i)) continue;
          if (dynamic_cast<Number*>(c) && is_callee) continue;
          *item = c;
          rewrote = true;
        }

        if (auto* o = dynamic_cast<Instruction_op*>(i)) {
          auto* l = dynamic_cast<Number*>(o->lhs_);
          auto* r = dynamic_cast<Number*>(o->rhs_);
          if (l && r) {
            i = new Instruction_assignment(o->dst_, new Number(fold(o->op_, l->number_, r->number_)));
            rewrote = true;
          }
        } else if (auto* b = dynamic_cast<Instruction_break_cond*>(i)) {
          if (auto* n = dynamic_cast<Number*>(b->t_)) {
            i = new Instruction_break_uncond(n->number_ == 1 ? b->label1_ : b->label2_);
            rewrote = true;
          }
        }
        transfer(env, i);
      }
    }
    return rewrote;
  }

  /*
   * Copy propagation and algebraic simplification within a block.
   */

  static bool is_number(Item* item, int64_t n) {
    auto* x = dynamic_cast<Number*>(item);
    return x && x->number_ == n;
  }

  static Instruction* simplify(Instruction_op* o) {
    Item* lhs = o->lhs_;
    Item* rhs = o->rhs_;
    switch (o->op_) {
      case OP::plus:
        if (is_number(rhs, 0)) return new Instruction_assignment(o->dst_, lhs);
        if (is_number(lhs, 0)) return new Instruction_assignment(o->dst_, rhs);
        break;
      case OP::minus:
      case OP::left_shift:
      case OP::right_shift:
        if (is_number(rhs, 0)) return new Instruction_assignment(o->dst_, lhs);
        break;
      case OP::times:
        if (is_number(rhs, 1)) return new Instruction_assignment(o->dst_, lhs);
        if (is_number(lhs, 1)) return new Instruction_assignment(o->dst_, rhs);
        if (is_number(rhs, 0) || is_number(lhs, 0)) return new Instruction_assignment(o->dst_, new Number(0));
        break;
      case OP::at:
        if (is_number(rhs, -1)) return new Instruction_assignment(o->dst_, lhs);
        if (is_number(lhs, -1)) return new Instruction_assignment(o->dst_, rhs);
        break;
      default:
        break;
    }
    return nullptr;
  }

  static bool propagate_copies(Function* f) {
    bool changed = false;
    for (auto* bb : f->basic_blocks) {
      std::map<std::string, std::string> copies;
      for (auto &i : bb->instructions) {
        Slots s = use_slots(i);
        for (auto** item : s.items) {
          auto* v = dynamic_cast<Variable*>(*item);
          if (!v) continue;
          auto it = copies.find(v->var_);
          if (it != copies.end()) {
            *item = new Variable(it->second);
            changed = true;
          }
        }
        for (auto** var : s.vars) {
          auto it = copies.find((*var)->var_);
          if (it != copies.end()) {
            *var = new Variable(it->second);
            changed = true;
          }
        }

        if (auto* o = dynamic_cast<Instruction_op*>(i)) {
          if (auto* simpler = simplify(o)) {
            i = simpler;
            changed = true;
          }
        }

        Variable* d = defined(i);
        if (!d) continue;
        for (auto it = copies.begin(); it != copies.end();) {
          if (it->first == d->var_ || it->second == d->var_) it = copies.erase(it);
          else ++it;
        }
        if (auto* a = dynamic_cast<Instruction_assignment*>(i)) {
          if (auto* src = dynamic_cast<Variable*>(a->src_)) {
            auto st = f->variable_types.find(src->var_);
            auto dt = f->variable_types.find(d->var_);
            bool same_type = st != f->variable_types.end() && dt != f->variable_types.end() && st->second == dt->second;
            if (src->var_ != d->var_ && same_type) {
              copies[d->var_] = src->var_;
            }
          }
        }
      }
    }
    return changed;
  }

  /*
   * Constant offset folding within a block: x <- y + c relations are forwarded
   * into later additions so encode/decode adjustments cancel out.
   */

  struct Offset {
    std::string base;
    int64_t c;
  };

  static bool offset_form(Instruction* i, std::string &dst, std::string &base, int64_t &c) {
    auto* o = dynamic_cast<Instruction_op*>(i);
    if (!o || (o->op_ != OP::plus && o->op_ != OP::minus)) return false;
    auto* v = dynamic_cast<Variable*>(o->lhs_);
    auto* n = dynamic_cast<Number*>(o->rhs_);
    if (!v || !n) {
      if (o->op_ != OP::plus) return false;
      v = dynamic_cast<Variable*>(o->rhs_);
      n = dynamic_cast<Number*>(o->lhs_);
      if (!v || !n) return false;
    }
    dst = o->dst_->var_;
    base = v->var_;
    c = o->op_ == OP::plus ? n->number_ : -n->number_;
    return true;
  }

  static Instruction* make_offset(Variable* dst, const std::string &base, int64_t c) {
    if (c == 0) return new Instruction_assignment(dst, new Variable(base));
    if (c < 0) return new Instruction_op(dst, new Variable(base), OP::minus, new Number(-c));
    return new Instruction_op(dst, new Variable(base), OP::plus, new Number(c));
  }

  static Variable* fresh_variable(Function* f) {
    static int64_t counter = 0;
    std::string name;
    do {
      name = "%__ofs" + std::to_string(counter++);
    } while (f->variable_types.count(name));
    f->variable_types[name] = {Type::int64, 0};
    return new Variable(name);
  }

  static bool fold_offsets(Function* f) {
    bool changed = false;
    for (auto* bb : f->basic_blocks) {
      std::map<std::string, Offset> offsets;
      std::vector<Instruction*> out;
      for (auto* i : bb->instructions) {
        std::vector<Instruction*> emitted = {i};

        if (auto* o = dynamic_cast<Instruction_op*>(i)) {
          auto offset_of = [&](Item* item, Offset &off) {
            auto* v = dynamic_cast<Variable*>(item);
            if (!v) return false;
            auto it = offsets.find(v->var_);
            if (it == offsets.end()) return false;
            off = it->second;
            return true;
          };
          Offset l, r;
          auto* rn = dynamic_cast<Number*>(o->rhs_);
          if ((o->op_ == OP::plus || o->op_ == OP::minus) && rn && offset_of(o->lhs_, l)) {
            int64_t c = o->op_ == OP::plus ? l.c + rn->number_ : l.c - rn->number_;
            emitted = {make_offset(o->dst_, l.base, c)};
          } else if (o->op_ == OP::plus && offset_of(o->rhs_, r) && !dynamic_cast<Number*>(o->lhs_)) {
            Variable* partial = fresh_variable(f);
            emitted = {
              new Instruction_op(partial, o->lhs_, OP::plus, new Variable(r.base)),
              make_offset(o->dst_, partial->var_, r.c)
            };
          } else if (o->op_ == OP::plus && offset_of(o->lhs_, l) && !dynamic_cast<Number*>(o->rhs_)) {
            Variable* partial = fresh_variable(f);
            emitted = {
              new Instruction_op(partial, new Variable(l.base), OP::plus, o->rhs_),
              make_offset(o->dst_, partial->var_, l.c)
            };
          } else if (o->op_ == OP::minus && offset_of(o->lhs_, l) && !dynamic_cast<Number*>(o->rhs_)) {
            Variable* partial = fresh_variable(f);
            emitted = {
              new Instruction_op(partial, new Variable(l.base), OP::minus, o->rhs_),
              make_offset(o->dst_, partial->var_, l.c)
            };
          }
        }

        for (auto* e : emitted) {
          std::string dst, base;
          int64_t c;
          std::string pdst, pbase;
          int64_t pc;
          if (!out.empty() && offset_form(e, dst, base, c) && base == dst
              && offset_form(out.back(), pdst, pbase, pc) && pdst == dst) {
            out.back() = make_offset(defined(out.back()), pbase, pc + c);
            changed = true;
          } else {
            out.push_back(e);
          }
          if (e != i) changed = true;

          Instruction* last = out.back();
          if (Variable* d = defined(last)) {
            for (auto it = offsets.begin(); it != offsets.end();) {
              if (it->first == d->var_ || it->second.base == d->var_) it = offsets.erase(it);
              else ++it;
            }
            if (offset_form(last, dst, base, c) && base != dst) {
              offsets[dst] = Offset{base, c};
            }
          }
        }
      }
      bb->instructions = out;
    }
    return changed;
  }

  /*
   * Dead code elimination.
   */

  static bool eliminate_dead_code(Function* f) {
    std::map<BasicBlock*, std::set<std::string>> live_in;
    std::map<BasicBlock*, std::set<std::string>> live_out;

    bool changed = true;
    while (changed) {
      changed = false;
      for (auto it = f->basic_blocks.rbegin(); it != f->basic_blocks.rend(); ++it) {
        BasicBlock* bb = *it;
        std::set<std::string> live;
        for (auto* s : bb->succs) live.insert(live_in[s].begin(), live_in[s].end());
        live_out[bb] = live;
        for (auto i = bb->instructions.rbegin(); i != bb->instructions.rend(); ++i) {
          if (Variable* d = defined(*i)) live.erase(d->var_);
          for (auto &u : used_names(*i)) live.insert(u);
        }
        if (live != live_in[bb]) {
          live_in[bb] = live;
          changed = true;
        }
      }
    }

    bool removed = false;
    for (auto* bb : f->basic_blocks) {
      std::set<std::string> live = live_out[bb];
      std::vector<Instruction*> kept;
      for (auto i = bb->instructions.rbegin(); i != bb->instructions.rend(); ++i) {
        Instruction* inst = *i;
        Variable* d = defined(inst);
        if (d && !live.count(d->var_)) {
          if (removable(inst)) {
            removed = true;
            continue;
          }
          if (auto* call = dynamic_cast<Instruction_call_assignment*>(inst)) {
            if (call->c_ != CallType::input) {
              inst = new Instruction_call(call->c_, call->callee_, call->args_);
              removed = true;
            }
          }
        }
        if (auto* a = dynamic_cast<Instruction_assignment*>(inst)) {
          auto* src = dynamic_cast<Variable*>(a->src_);
          if (src && src->var_ == a->dst_->var_) {
            removed = true;
            continue;
          }
        }
        if (Variable* dd = defined(inst)) live.erase(dd->var_);
        for (auto &u : used_names(inst)) live.insert(u);
        kept.push_back(inst);
      }
      std::reverse(kept.begin(), kept.end());
      bb->instructions = kept;
    }
    return removed;
  }

  /*
   * Control flow simplification.
   */

  static bool simplify_cfg(Function* f) {
    bool changed = false;

    for (auto* bb : f->basic_blocks) {
      auto* c = dynamic_cast<Instruction_break_cond*>(terminator(bb));
      if (c && c->label1_->label_ == c->label2_->label_) {
        bb->instructions.back() = new Instruction_break_uncond(c->label1_);
        changed = true;
      }
    }
    rebuild_cfg(f);

    std::map<std::string, std::string> forward;
    for (size_t k = 1; k < f->basic_blocks.size(); k++) {
      BasicBlock* bb = f->basic_blocks[k];
      if (bb->instructions.size() != 1) continue;
      auto* u = dynamic_cast<Instruction_break_uncond*>(bb->instructions[0]);
      if (u && u->label_->label_ != bb->label_->label_) forward[bb->label_->label_] = u->label_->label_;
    }
    auto resolve = [&](const std::string &label) {
      std::string l = label;
      std::set<std::string> seen;
      while (forward.count(l) && seen.insert(l).second) l = forward[l];
      return l;
    };
    for (auto* bb : f->basic_blocks) {
      Instruction* t = terminator(bb);
      if (auto* u = dynamic_cast<Instruction_break_uncond*>(t)) {
        std::string target = resolve(u->label_->label_);
        if (target != u->label_->label_) {
          u->label_ = new Label(target);
          changed = true;
        }
      } else if (auto* c = dynamic_cast<Instruction_break_cond*>(t)) {
        std::string t1 = resolve(c->label1_->label_);
        std::string t2 = resolve(c->label2_->label_);
        if (t1 != c->label1_->label_ || t2 != c->label2_->label_) {
          c->label1_ = new Label(t1);
          c->label2_ = new Label(t2);
          changed = true;
        }
      }
    }
    rebuild_cfg(f);

    std::set<BasicBlock*> reachable;
    std::vector<BasicBlock*> work = {f->basic_blocks[0]};
    while (!work.empty()) {
      BasicBlock* bb = work.back();
      work.pop_back();
      if (!reachable.insert(bb).second) continue;
      for (auto* s : bb->succs) work.push_back(s);
    }
    std::vector<BasicBlock*> kept;
    for (auto* bb : f->basic_blocks) {
      if (reachable.count(bb)) kept.push_back(bb);
      else changed = true;
    }
    f->basic_blocks = kept;
    rebuild_cfg(f);

    auto preds = predecessors(f);
    for (size_t k = 0; k < f->basic_blocks.size(); k++) {
      BasicBlock* bb = f->basic_blocks[k];
      auto* u = dynamic_cast<Instruction_break_uncond*>(terminator(bb));
      if (!u) continue;
      BasicBlock* next = f->label_to_bb.at(u->label_->label_);
      if (next == bb || next == f->basic_blocks[0] || preds[next].size() != 1) continue;
      bb->instructions.pop_back();
      bb->instructions.insert(bb->instructions.end(), next->instructions.begin(), next->instructions.end());
      f->basic_blocks.erase(std::find(f->basic_blocks.begin(), f->basic_blocks.end(), next));
      rebuild_cfg(f);
      preds = predecessors(f);
      changed = true;
      k--;
    }
    return changed;
  }

  void optimize(Program& p) {
    for (auto* f : p.functions) {
      rebuild_cfg(f);
      for (int round = 0; round < 16; round++) {
        bool changed = false;
        changed |= propagate_constants(f);
        changed |= propagate_copies(f);
        changed |= fold_offsets(f);
        changed |= eliminate_dead_code(f);
        changed |= simplify_cfg(f);
        rebuild_cfg(f);
        if (!changed) break;
      }
    }
  }

}
