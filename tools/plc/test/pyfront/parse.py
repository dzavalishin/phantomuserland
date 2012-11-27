import tokenize
from tokenize import Token
if '.' in str(1.0):
    from boot import *

def check(t,*vs):
    if vs[0] == None: return True
    if t.type in vs: return True
    if t.type == 'symbol' and t.val in vs: return True
    return False

def tweak(k,v):
    P.stack.append((k,dmap[k]))
    if v: dmap[k] = omap[k]
    else: dmap[k] = {'lbp':0,'nud':itself}
def restore():
    k,v = P.stack.pop()
    dmap[k] = v

def cpy(d):
    r = {}
    for k in d: r[k] = d[k]
    return r

class PData:
    def __init__(self,s,tokens):
        self.s = s
        self.tokens = tokens
        self.pos = 0
        self.token = None
        self.stack = []
    def init(self):
        global omap,dmap
        omap = cpy(base_dmap)
        dmap = cpy(base_dmap)
        self.advance()
    def advance(self,val=None):
        if not check(self.token,val):
            error('expected '+val,self.token)
        if self.pos < len(self.tokens):
            t = self.tokens[self.pos]
            self.pos += 1
        else:
            t = Token((0,0),'eof','eof')
        self.token = do(t)
        return t
def error(ctx,t):
    print t
    tokenize.u_error(ctx,P.s,t.pos)

def nud(t):
    #if 'nud' not in t:
        #error('no nud',t)
    return t.nud(t)
def led(t,left):
    #if 'led' not in t:
        #error('no led',t)
    return t.led(t,left)
def get_lbp(t):
    #if 'lbp' not in t:
        #error('no lbp',t)
    return t.lbp
def get_items(t):
    #if 'items' not in t:
        #error('no items',t)
    return t.items

def expression(rbp):
    t = P.token
    advance()
    left = nud(t)
    while rbp < get_lbp(P.token):
        t = P.token
        advance()
        left = led(t,left)
    return left

def infix_led(t,left):
    t.items = [left,expression(t.bp)]
    return t
def infix_is(t,left):
    if check(P.token,'not'):
        t.val = 'isnot'
        advance('not')
    t.items = [left,expression(t.bp)]
    return t
def infix_not(t,left):
    advance('in')
    t.val = 'notin'
    t.items = [left,expression(t.bp)]
    return t
def infix_tuple(t,left):
    r = expression(t.bp)
    if left.val == ',':
        left.items.append(r)
        return left
    t.items = [left,r]
    t.type = 'tuple'
    return t
def lst(t):
    if t == None: return []
    if check(t,',','tuple','statements'):
        return get_items(t)
    return [t]
def ilst(typ,t):
    return Token(t.pos,typ,typ,lst(t))

def call_led(t,left):
    r = Token(t.pos,'call','$',[left])
    while not check(P.token,')'):
        tweak(',',0)
        r.items.append(expression(0))
        if P.token.val == ',': advance(',')
        restore()
    advance(")")
    return r
def get_led(t,left):
    r = Token(t.pos,'get','.',[left])
    items =  [left]
    more = False
    while not check(P.token,']'):
        more = False
        if check(P.token,':'):
            items.append(Token(P.token.pos,'symbol','None'))
        else:
            items.append(expression(0))
        if check(P.token,':'):
            advance(':')
            more = True
    if more:
        items.append(Token(P.token.pos,'symbol','None'))
    if len(items) > 2:
        items = [left,Token(t.pos,'slice',':',items[1:])]
    r.items = items
    advance("]")
    return r
def dot_led(t,left):
    r = expression(t.bp)
    r.type = 'string'
    t.items = [left,r]
    return t

def itself(t):
    return t
def paren_nud(t):
    tweak(',',1)
    r = expression(0)
    restore()
    advance(')')
    return r
def list_nud(t):
    t.type = 'list'
    t.val = '[]'
    t.items = []
    next = P.token
    tweak(',',0)
    while not check(P.token,'for',']'):
        r = expression(0)
        t.items.append(r)
        if P.token.val == ',': advance(',')
    if check(P.token,'for'):
        t.type = 'comp'
        advance('for')
        tweak('in',0)
        t.items.append(expression(0))
        advance('in')
        t.items.append(expression(0))
        restore()
    restore()
    advance(']')
    return t
def dict_nud(t):
    t.type='dict'
    t.val = '{}'
    t.items = []
    tweak(',',0)
    while not check(P.token,'}'):
        t.items.append(expression(0))
        if check(P.token,':',','): advance()
    restore()
    advance('}')
    return t

def advance(t=None):
    return P.advance(t)

def block():
    items = []
    tok = P.token

    while check(P.token,'nl'): advance()
    if check(P.token,'indent'):
        advance('indent')
        while not check(P.token,'dedent'):
            items.append(expression(0))
            while check(P.token,';','nl'): advance()
        advance('dedent')
    else:
        items.append(expression(0))
        while check(P.token,';'):
            advance(';')
            items.append(expression(0))
    while check(P.token,'nl'): advance()

    if len(items) > 1:
        return Token(tok.pos,'statements',';',items)
    return items.pop()

def def_nud(t):
    items = t.items = []
    items.append(P.token); advance()
    advance('(')
    r = Token(t.pos,'symbol','():',[])
    items.append(r)
    while not check(P.token,')'):
        tweak(',',0)
        r.items.append(expression(0))
        if check(P.token,','): advance(',')
        restore()
    advance(')')
    advance(':')
    items.append(block())
    return t


def while_nud(t):
    items = t.items = []
    items.append(expression(0))
    advance(':')
    items.append(block())
    return t
def class_nud(t):
    items = t.items = []
    items.append(expression(0))
    advance(':')
    items.append(ilst('methods',block()))
    return t

def from_nud(t):
    items = t.items = []
    items.append(expression(0))
    advance('import')
    items.append(expression(0))
    return t

def for_nud(t):
    items = t.items = []
    tweak('in',0)
    items.append(expression(0))
    advance('in')
    items.append(expression(0))
    restore()
    advance(':')
    items.append(block())
    return t
def if_nud(t):
    items = t.items = []
    a = expression(0)
    advance(':')
    b = block()
    items.append(Token(t.pos,'elif','elif',[a,b]))
    while check(P.token,'elif'):
        tok = P.token
        advance('elif')
        a = expression(0)
        advance(':')
        b = block()
        items.append(Token(tok.pos,'elif','elif',[a,b]))
    if check(P.token,'else'):
        tok = P.token
        advance('else')
        advance(':')
        b = block()
        items.append(Token(tok.pos,'else','else',[b]))
    return t
def try_nud(t):
    items = t.items = []
    advance(':')
    b = block()
    items.append(b)
    while check(P.token,'except'):
        tok = P.token
        advance('except')
        if not check(P.token,':'): a = expression(0)
        else: a = Token(tok.pos,'symbol','None')
        advance(':')
        b = block()
        items.append(Token(tok.pos,'except','except',[a,b]))
    if check(P.token,'else'):
        tok = P.token
        advance('else')
        advance(':')
        b = block()
        items.append(Token(tok.pos,'else','else',[b]))
    return t
def prefix_nud(t):
    #bp = 70
    #if 'bp' in t: bp = t['bp']
    bp = t.bp
    t.items = [expression(bp)]
    return t
def prefix_nud0(t):
    if check(P.token,'nl',';','eof','dedent'): return t
    return prefix_nud(t)
def prefix_nuds(t):
    r = expression(0)
    return ilst(t.type,r)

def prefix_neg(t):
    r = expression(50)
    if r.type == 'number':
        r.val = str(-float(r.val))
        return r
    t.items = [Token(t.pos,'number','0'),r]
    return t
def vargs_nud(t):
    r = prefix_nud(t)
    t.type = 'args'
    t.val = '*'
    return t
def nargs_nud(t):
    r = prefix_nud(t)
    t.type = 'nargs'
    t.val = '**'
    return t


base_dmap = {
    ',':{'lbp':20,'bp':20,'led':infix_tuple},
    '+':{'lbp':50,'bp':50,'led':infix_led},
    '-':{'lbp':50,'nud':prefix_neg,
        'bp':50,'led':infix_led},
    'not':{'lbp':35,'nud':prefix_nud,'bp':35,
        'bp':35,'led':infix_not },
    '%':{'lbp':60,'bp':60,'led':infix_led},
    '*':{'lbp':60,'nud':vargs_nud,
        'bp':60,'led':infix_led,},
    '**': {'lbp':65,'nud':nargs_nud,
        'bp':65,'led':infix_led,},
    '/':{'lbp':60,'bp':60,'led':infix_led},
    '(':{'lbp':70,'nud':paren_nud,
        'bp':80,'led':call_led,},
    '[':{'lbp':70,'nud':list_nud,
        'bp':80,'led':get_led,},
    '{':{'lbp':0,'nud':dict_nud,},
    '.':{'lbp':80,'bp':80,'led':dot_led,'type':'get',},
    'break':{'lbp':0,'nud':itself,'type':'break'},
    'pass':{'lbp':0,'nud':itself,'type':'pass'},
    'continue':{'lbp':0,'nud':itself,'type':'continue'},
    'eof':{'lbp':0,'type':'eof','val':'eof'},
    'def':{'lbp':0,'nud':def_nud,'type':'def',},
    'while':{'lbp':0,'nud':while_nud,'type':'while',},
    'for':{'lbp':0,'nud':for_nud,'type':'for',},
    'try':{'lbp':0,'nud':try_nud,'type':'try',},
    'if':{'lbp':0,'nud':if_nud,'type':'if',},
    'class':{'lbp':0,'nud':class_nud,'type':'class',},
    'raise':{'lbp':0,'nud':prefix_nud0,'type':'raise','bp':20,},
    'return':{'lbp':0,'nud':prefix_nud0,'type':'return','bp':10,},
    'import':{'lbp':0,'nud':prefix_nuds,'type':'import','bp':20,},
    'from':{'lbp':0,'nud':from_nud,'type':'from','bp':20,},
    'del':{'lbp':0,'nud':prefix_nuds,'type':'del','bp':10,},
    'global':{'lbp':0,'nud':prefix_nuds,'type':'globals','bp':20,},

    '=':{
        'lbp':10,'bp':9,'led':infix_led,
        },
}

def i_infix(bp,led,*vs):
    for v in vs: base_dmap[v] = {'lbp':bp,'bp':bp,'led':led}
i_infix(40,infix_led,'<','>','<=','>=','!=','==')
i_infix(40,infix_is,'is','in')
i_infix(10,infix_led,'+=','-=','*=','/=')
i_infix(31,infix_led,'and','&')
i_infix(30,infix_led,'or','|')
i_infix(36,infix_led,'<<','>>')
def i_terms(*vs):
    for v in vs: base_dmap[v] = {'lbp':0,'nud':itself}
i_terms(')','}',']',';',':','nl','elif','else','True','False','None','name','string','number','indent','dedent','except')
base_dmap['nl']['val'] = 'nl'

def gmap(t,v):
    if v not in dmap:
        error('unknown "%s"'%v,t)
    return dmap[v]

def do(t):
    if t.type == 'symbol': r = gmap(t,t.val)
    else: r = gmap(t,t.type)
    merge(t,r)
    return t
def do_module():
    tok = P.token
    items = []
    while not check(P.token,'eof'):
        items.append(block())
    if len(items) > 1:
        return Token(tok.pos,'statements',';',items)
    return items.pop()

def parse(s,tokens,wrap=0):
    global P
    s = tokenize.clean(s)
    P=PData(s,tokens); P.init()
    r = do_module()
    P = None
    return r

