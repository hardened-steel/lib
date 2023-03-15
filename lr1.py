from typing import List, Set, Tuple
from collections import defaultdict
from enum import Enum

class Action(Enum):
    goto = 1
    reduce = 2
    shift = 3
    accept = 4
    error = 5

class Symbol:
    def __init__(self, name):
        self.name = name
        
    def __str__(self):
        return self.name
    
    def __repr__(self):
        return self.name
    
    def __eq__(self, other):
        return isinstance(other, Symbol) and self.name == other.name
    
    def __hash__(self):
        return hash(self.name)

    def __iter__(self):
        return iter((self.name))
    
class Terminal(Symbol):
    pass

class NonTerminal(Symbol):
    pass

class Production:
    def __init__(self, lhs: NonTerminal, rhs: List[Symbol]):
        self.lhs = lhs
        self.rhs = rhs
        
    def __str__(self):
        rhs_str = " ".join(str(s) for s in self.rhs)
        return f"{self.lhs} := {rhs_str}"

    def __hash__(self):
        return hash((self.lhs, tuple(self.rhs)))
    
    def __repr__(self):
        return str(self)
    
    def __iter__(self):
        return iter((self.lhs, self.rhs))

class Item:
    def __init__(self, production, lookahead, dot_pos):
        assert(isinstance(production, Production))
        #assert(isinstance(lookahead, Terminal))

        self.production = production
        self.lookahead = lookahead
        self.dot_pos = dot_pos

    def __eq__(self, other):
        return isinstance(other, Item) and self.production == other.production and self.lookahead == other.lookahead and self.dot_pos == other.dot_pos

    def __hash__(self):
        return hash((self.production, self.lookahead, self.dot_pos))

    def __str__(self):
        lhs = str(self.production.lhs)
        rhs = [str(sym) for sym in self.production.rhs]
        rhs.insert(self.dot_pos, ".")
        return f"({lhs} := {' '.join(rhs)} , \'{self.lookahead}\')"

    def __repr__(self):
        return self.__str__()
    
    def __iter__(self):
        return iter((self.production, self.dot_pos, self.lookahead))


def BUILD_FIRST_SETS(grammar):
    first_sets = {}
    first_sets = defaultdict(set)
    changed = True
    while changed:
        changed = False
        for production in grammar.productions:
            head, body = production.lhs, production.rhs
            head_set = first_sets[head]
            if len(body) > 0:
                symbol = body[0]
                if isinstance(symbol, Terminal):
                    # Если символ - терминал, добавляем его в FIRST
                    if symbol not in head_set:
                        head_set.add(symbol)
                        changed = True
                else:
                    # Если символ - нетерминал, добавляем FIRST(B) в FIRST(A)
                    for item in first_sets[symbol]:
                        if item not in head_set:
                            head_set.add(item)
                            changed = True
            else:
                symbol = Terminal('ε')
                if symbol not in head_set:
                    head_set.add(symbol)
                    changed = True
    return first_sets

def FIRST(sequence, lookahead, grammar, first_sets):
    epsilon = Terminal('ε')
    result = set()
    if not len(sequence):
        result.add(lookahead)
        return result

    for symbol in sequence:
        if isinstance(symbol, Terminal):
            result.add(symbol)
            return result
        stop = True
        for item in first_sets[symbol]:
            if item is epsilon:
                stop = False
            else:
                result.add(item)
        if stop:
            if (len(result) > 1) and (epsilon in result):
                result.remove(epsilon)
            if (len(result) == 0):
                result.add(epsilon)
            return result

def CLOSURE(closure_items, grammar, first_sets):
    changed = True
    max_items = 0
    while changed:
        changed = False
        for item in closure_items:
            rule, dot_pos, lookahead = item
            sequence = rule.rhs
            if dot_pos < len(sequence) and isinstance(sequence[dot_pos], NonTerminal):
                first = FIRST(sequence[dot_pos + 1:], lookahead, grammar, first_sets)
                for production in grammar.productions:
                    if production.lhs == sequence[dot_pos]:
                        for terminal in first:
                            new_item = Item(production, terminal, 0)
                            max_items += 1
                            if new_item not in closure_items:
                                closure_items.append(new_item)
                                changed = True
    print(f'max closure items: {max_items}')
    return closure_items

def GOTO(items, symbol, grammar, first_sets):
    goto_items = []
    for rule, dot_pos, lookahead in items:
        sequence = rule.rhs
        if dot_pos < len(sequence) and sequence[dot_pos] == symbol:
            new_item = Item(rule, lookahead, dot_pos + 1)
            goto_items.append(new_item)
    return CLOSURE(goto_items, grammar, first_sets)

def BUILD_AUTOMATION(grammar):
    first_sets = BUILD_FIRST_SETS(grammar)
    if True:
        print("first table:")
        for rule, sequence in first_sets.items():
            print(f'\t{rule} => {sequence}')
    states = [CLOSURE([Item(grammar.productions[0], Terminal("$"), 0)], grammar, first_sets)]
    goto_sets = defaultdict(set)
    transitions = []
    reduces = []
    changed = True
    while changed:
        changed = False
        for i, state in enumerate(states):
            for item in state:
                rule, dot_pos, lookahead = item
                sequence = rule.rhs
                if dot_pos < len(sequence):
                    next_symbol = sequence[dot_pos]
                    next_state = goto_sets[(i, next_symbol)]
                    if next_state == set():
                        print(f'goto({i}, {next_symbol})')
                        next_state = GOTO(state, next_symbol, grammar, first_sets)
                        goto_sets[(i, next_symbol)] = next_state
                    if next_state not in states:
                        states.append(next_state)
                        changed = True
                    next_transition = (i, next_symbol, states.index(next_state))
                    if next_transition not in transitions:
                        transitions.append(next_transition)
                        changed = True
                if dot_pos is len(sequence):
                    reduce = (i, lookahead, rule)
                    if reduce not in reduces:
                        reduces.append((i, lookahead, rule))
                        changed = True
    return states, transitions, reduces

def BUILD_TABLE(grammar):
    states, transitions, reduces = BUILD_AUTOMATION(grammar)

    if True:
        print("states:")
        for i, state in enumerate(states):
            print(f'\tstate[{i}, {len(state)}]: {state}')

        print("transitions:")
        for i, transition in enumerate(transitions):
            print(f'\ttransition: {transition}')

        print("reduces:")
        for state, lookahead, rule in reduces:
            print(f'\tstate is {state}, current symbol is \'{lookahead}\' => \"{rule}\" [{grammar.productions.index(rule)}]')

    action_table = defaultdict(lambda: (Action.error, None, f'error'))
    goto_table = {}

    for state, lookahead, rule in reduces:
        key = (state, lookahead)
        action_table[key] = (Action.reduce, rule, f'reduce-{grammar.productions.index(rule)}')

    for transition in transitions:
        begin, symbol, end = transition
        if isinstance(symbol, Terminal):
            key = (begin, symbol)
            if key in action_table:
                _, _, title = action_table[key]
                raise Exception(f'(shift-{end})-({title}) conflict')
            else:
                action_table[key] = (Action.shift, end, f'shift-{end}')
        else:
            goto_table[(begin, symbol)] = (Action.goto, end, f'goto {end}')

    for i, state in enumerate(states):
        for item in state:
            rule, dot_pos, lookahead = item
            nonterminal = rule.lhs
            sequence = rule.rhs
            if dot_pos == len(sequence):
                if nonterminal == NonTerminal("S'") and rule == grammar.productions[0]:
                    key = (i, Terminal("$"))
                    if key in action_table:
                        action, _, title = action_table[key]
                        if not action is Action.reduce:
                            raise Exception(f'(accept)-({title}) conflict')
                    action_table[key] = (Action.accept, None, f'accept')

    return action_table, goto_table

def PARSE(grammar, sequence: list):
    action_table, goto_table = BUILD_TABLE(grammar)

    print("actions:")
    for key, value in action_table.items():
        state, symbol = key
        action, param, title = value
        print(f'\t{state} & \'{symbol}\' => \"{title}\"')

    print("goto:")
    for key, value in goto_table.items():
        state, symbol = key
        action, param, title = value
        print(f'\t{state} & \'{symbol}\' => \"{title}\"')

    stack = [0]
    for symbol in sequence:
        while True:
            state = stack[len(stack) - 1]
            action, param, title = action_table[(state, symbol)]
            print(f'symbol({symbol}), state({state}), action({title}), stack: {stack}')
            if action is Action.shift:
                next_state = param
                stack.append(symbol)
                stack.append(next_state)
                break
            if action is Action.reduce:
                rule = param
                nonterminal = rule.lhs
                symbols = []
                for item in rule.rhs:
                    stack.pop()
                    symbols.append(stack.pop())
                state = stack[len(stack) - 1]
                stack.append(nonterminal)
                stack.append(goto_table[(state, nonterminal)][1])
            if action is Action.accept:
                return stack.pop()
            if action is Action.error:
                raise Exception("parse error")

class Grammar:
    def __init__(self, productions: List[Production]):
        self.productions = productions
    
    def get_start_symbol(self):
        return self.productions[0].lhs
    
    def NonTerminals(self):
        NonTerminals = []
        for production in self.productions:
            NonTerminals.append(production.lhs)
        return NonTerminals
    
    def Terminals(self):
        terminals = set()
        for production in self.productions:
            for symbol in production.rhs:
                if isinstance(symbol, Terminal):
                    terminals.add(symbol)
        return terminals

# Определим терминал и нетерминал для грамматики
SheepNoise = NonTerminal("SheepNoise")
BA = Terminal("baa")

# Определим продукции для грамматики
productions_ = [
    Production(NonTerminal("S'"), [SheepNoise]),
    Production(SheepNoise, [BA, SheepNoise]),
    Production(SheepNoise, [BA])
]

productions = [
    Production(NonTerminal("S'"), [NonTerminal("E")]),
    Production(NonTerminal("E"), [NonTerminal("E"), Terminal("+"), NonTerminal("T")]),
    Production(NonTerminal("E"), [NonTerminal("T")]),
    Production(NonTerminal("T"), [NonTerminal("T"), Terminal("*"), NonTerminal("F")]),
    Production(NonTerminal("T"), [NonTerminal("F")]),
    Production(NonTerminal("F"), [Terminal("("), NonTerminal("E"), Terminal(")")]),
    Production(NonTerminal("F"), [Terminal("id")]),
]

productions2 = [
    Production(NonTerminal("S'"), [NonTerminal("S")]),
    Production(NonTerminal("S"), [NonTerminal("C"), NonTerminal("C")]),
    Production(NonTerminal("C"), [Terminal("c"), NonTerminal("C")]),
    Production(NonTerminal("C"), [Terminal("d")]),
]

# Создадим объект грамматики
#sheep_grammar = Grammar(productions_)
#sequence = [BA, BA, BA, Terminal("$")]
PARSE(Grammar(productions_), [BA, BA, BA, Terminal("$")])

PARSE(Grammar(productions), [Terminal("id"), Terminal("+"), Terminal("id"), Terminal("*"), Terminal("("), Terminal("id"), Terminal("+"), Terminal("id"), Terminal(")"), Terminal("$")])
#PARSE(Grammar(productions), [Terminal("id"), Terminal("*"), Terminal("id"), Terminal("+"), Terminal("id"), Terminal("$")])
PARSE(Grammar(productions2), [Terminal("c"), Terminal("d"), Terminal("d"), Terminal("$")])
