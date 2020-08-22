# figure out if we're in python or tinypy (tinypy displays "1.0" as "1")
is_tinypy = (str(1.0) == "1")
if not is_tinypy:
    from boot import *

################################################################################
RM = 'rm -f '
VM = './vm '
TINYPY = './tinypy '
TMP = 'tmp.txt'
if '-mingw32' in ARGV or "-win" in ARGV:
    RM = 'del '
    VM = 'vm '
    TINYPY = 'tinypy '
    TMP = 'tmp.txt'
    #TMP = 'stdout.txt'
def system_rm(fname):
    system(RM+fname)

################################################################################
#if not is_tinypy:
    #v = chksize()
    #assert (v < 65536)

################################################################################
def t_show(t):
    if t.type == 'string': return '"'+t.val+'"'
    if t.type == 'number': return t.val
    if t.type == 'symbol': return t.val
    if t.type == 'name': return '$'+t.val
    return t.type
def t_tokenize(s,exp=''):
    import tokenize
    result = tokenize.tokenize(s)
    res = ' '.join([t_show(t) for t in result])
    #print(s); print(exp); print(res)
    assert(res == exp)

if __name__ == '__main__':
    t_tokenize("234",'234')
    t_tokenize("234.234",'234.234')
    t_tokenize("phil",'$phil')
    t_tokenize("_phil234",'$_phil234')
    t_tokenize("'phil'",'"phil"')
    t_tokenize('"phil"','"phil"')
    t_tokenize("'phil' x",'"phil" $x')
    t_tokenize("#comment","")
    t_tokenize("and","and")
    t_tokenize("=","=")
    t_tokenize("()","( )")
    t_tokenize("(==)","( == )")
    t_tokenize("phil=234","$phil = 234")
    t_tokenize("a b","$a $b")
    t_tokenize("a\nb","$a nl $b")
    t_tokenize("a\n    b","$a nl indent $b dedent")
    t_tokenize("a\n    b\n        c", "$a nl indent $b nl indent $c dedent dedent")
    t_tokenize("a\n    b\n    c", "$a nl indent $b nl $c dedent")
    t_tokenize("a\n    b\n        \n      c", "$a nl indent $b nl nl indent $c dedent dedent")
    t_tokenize("a\n    b\nc", "$a nl indent $b nl dedent $c")
    t_tokenize("a\n  b\n    c\nd", "$a nl indent $b nl indent $c nl dedent dedent $d")
    t_tokenize("(\n  )","( )")
    t_tokenize("  x","indent $x dedent")
    t_tokenize("  #","")
    t_tokenize("None","None")


################################################################################

def t_lisp(t):
    if t.type == 'block':
        return """{%s}"""%' '.join([t_lisp(tt) for tt in t.items])
    if t.type == 'statement':
        return """%s;"""%' '.join([t_lisp(tt) for tt in t.items])
    if t.items == None: return t.val
    args = ''.join([" "+t_lisp(tt) for tt in t.items])
    return "("+t.val+args+")"

def t_parse(s,ex=''):
    import tokenize, parse
    r = ''
    tokens = tokenize.tokenize(s)
    tree = parse.parse(s,tokens)
    r = t_lisp(tree)
    #print(s); print(ex); print(r)
    assert(r==ex)

if __name__ == '__main__':
    t_parse('2+4*3', '(+ 2 (* 4 3))')
    t_parse('4*(2+3)', '(* 4 (+ 2 3))')
    t_parse('(2+3)*4', '(* (+ 2 3) 4)')
    t_parse('1<2', '(< 1 2)')
    t_parse('x=3', '(= x 3)')
    t_parse('x = 2*3', '(= x (* 2 3))')
    t_parse('x = y', '(= x y)')
    t_parse('2,3', '(, 2 3)')
    t_parse('2,3,4', '(, 2 3 4)')
    t_parse('[]', '([])')
    t_parse('[1]', '([] 1)')
    t_parse('[2,3,4]', '([] 2 3 4)')
    t_parse('print(3)', '($ print 3)')
    t_parse('print()', '($ print)')
    t_parse('print(2,3)', '($ print 2 3)')
    t_parse('def fnc():pass', '(def fnc (():) pass)')
    t_parse('def fnc(x):pass', '(def fnc ((): x) pass)')
    t_parse('def fnc(x,y):pass', '(def fnc ((): x y) pass)')
    t_parse('x\ny\nz', '(; x y z)')
    t_parse('return x', '(return x)')
    t_parse('print(test(2,3))', '($ print ($ test 2 3))')
    t_parse('x.y', '(. x y)')
    t_parse('get(2).x', '(. ($ get 2) x)')
    t_parse('{}', '({})')
    t_parse('True', 'True')
    t_parse('False', 'False')
    t_parse('None', 'None')
    t_parse('while 1:pass', '(while 1 pass)')
    t_parse('for x in y:pass', '(for x y pass)')
    t_parse('print("x")', '($ print x)')
    t_parse('if 1: pass', '(if (elif 1 pass))')
    t_parse('x = []', '(= x ([]))')
    t_parse('x[1]', '(. x 1)')
    t_parse('import sdl', '(import sdl)')
    t_parse('2 is 3', '(is 2 3)')
    t_parse('2 is not 3', '(isnot 2 3)')
    t_parse('x is None', '(is x None)')
    t_parse('2 is 3 or 4 is 5', '(or (is 2 3) (is 4 5))')
    t_parse('e.x == 3 or e.x == 4', '(or (== (. e x) 3) (== (. e x) 4))')
    t_parse('if 1==2: 3\nelif 4:5\nelse:6', '(if (elif (== 1 2) 3) (elif 4 5) (else 6))')
    t_parse('x = y(2)*3 + y(4)*5', '(= x (+ (* ($ y 2) 3) (* ($ y 4) 5)))')
    t_parse('x(1,2)+y(3,4)', '(+ ($ x 1 2) ($ y 3 4))')
    t_parse('x(a,b,c[d])', '($ x a b (. c d))')
    t_parse('x(1,2)*j+y(3,4)*k+z(5,6)*l', '(+ (+ (* ($ x 1 2) j) (* ($ y 3 4) k)) (* ($ z 5 6) l))')
    t_parse('a = b.x/c * 2 - 1', '(= a (- (* (/ (. b x) c) 2) 1))')
    t_parse('for x in y: z', '(for x y z)')

    t_parse('min(255,n*2)','($ min 255 (* n 2))')
    t_parse('c = pal[i*8]','(= c (. pal (* i 8)))')
    t_parse('{x:y,a:b}','({} x y a b)')
    t_parse('x[1:2:3]','(. x (: 1 2 3))')
    if is_tinypy: t_parse('x - -234','(- x -234)')
    else: t_parse('x - -234','(- x -234.0)')
    t_parse('x - -y','(- x (- 0 y))')
    t_parse('x = ((y*4)-2)','(= x (- (* y 4) 2))')
    
    if is_tinypy: t_parse('print([1,2,"OK",4][-3:3][1])','($ print (. (. ([] 1 2 OK 4) (: -3 3)) 1))')
    else: t_parse('print([1,2,"OK",4][-3:3][1])','($ print (. (. ([] 1 2 OK 4) (: -3.0 3)) 1))')
    
    t_parse('a,b = 1,2','(= (, a b) (, 1 2))')
    t_parse('class C: pass','(class C (methods pass))')
    t_parse('def test(*v): pass','(def test ((): (* v)) pass)')
    t_parse('def test(**v): pass','(def test ((): (** v)) pass)')
    t_parse('test(*v)','($ test (* v))')
    t_parse('test(**v)','($ test (** v))')
    t_parse('def test(x=y): pass','(def test ((): (= x y)) pass)')
    t_parse('test(x=y)','($ test (= x y))')
    t_parse('def test(y="K",x="Z"): pass','(def test ((): (= y K) (= x Z)) pass)')
    t_parse('return x+y','(return (+ x y))')
    t_parse('if "a" is not "b": pass','(if (elif (isnot a b) pass))')
    t_parse('z = 0\nfor x in y: pass','(; (= z 0) (for x y pass))')
    t_parse('for k in {"OK":0}: pass','(for k ({} OK 0) pass)')
    t_parse('print(test(10,3,z=50000,*[200],**{"x":4000}))','($ print ($ test 10 3 (= z 50000) (* ([] 200)) (** ({} x 4000))))')
    t_parse('x="OK";print(x)','(; (= x OK) ($ print x))')
    t_parse('[(1,3)]','([] (, 1 3))')
    t_parse('x[:]','(. x (: None None))')
    t_parse('x[:1]','(. x (: None 1))')
    t_parse('x[1:]','(. x (: 1 None))')
    t_parse('return\nx','(; return x)')
    t_parse('"""test"""','test')
    t_parse('return a,b','(return (, a b))')

################################################################################

def showerror(cmd, ss, ex, res):
    print(cmd)
    print("ss : '" + str(ss) + "'")
    print("ex : '" + str(ex) + "'")
    print("res: '" + str(res) + "'")

def t_render(ss,ex,exact=True):
    import tokenize, parse, encode
        
    if not istype(ss,'list'): ss =[ss]
    n = 1
    for s in ss:
        fname = 'tmp'+str(n)+'.tpc'
        system_rm(fname)
        tokens = tokenize.tokenize(s)
        t = parse.parse(s,tokens)
        r = encode.encode(fname,s,t)
        f = save(fname,r)
        n += 1
    system_rm('tmp.txt')
    cmd = VM + fname + " > tmp.txt"
    system(cmd)
    res = load(TMP).strip()
    #print(ss,ex,res)
    if exact:
        if res != ex: showerror(cmd, ss, ex, res)
        assert(res == ex)
    else: 
        if ex not in res: showerror(cmd, ss, ex, res)
        assert(ex in res)

def test_range():
    t_render("""print(str(range(4))[:5])""","<list")
    t_render("""print(len(range(4)))""","4")
    t_render("""print(range(4)[0])""","0")
    t_render("""print(range(4)[1])""","1")
    t_render("""print(range(4)[-1])""","3")

    t_render("""print(str(range(-4))[:5])""","<list")
    t_render("""print(len(range(-4)))""","0")

    t_render("""print(str(range(0,5,3))[:5])""","<list")
    t_render("""print(len(range(0,5,3)))""","2")
    t_render("""print(range(0,5,3)[0])""","0")
    t_render("""print(range(0,5,3)[1])""","3")
    t_render("""print(range(0,5,3)[-1])""","3")

    t_render("""print(str(range(5,0,-3))[:5])""","<list")
    t_render("""print(len(range(5,0,-3)))""","2")
    t_render("""print(range(5,0,-3)[0])""","5")
    t_render("""print(range(5,0,-3)[1])""","2")
    t_render("""print(range(5,0,-3)[-1])""","2")

    t_render("""print(str(range(-8,-4))[:5])""","<list")
    t_render("""print(len(range(-8,-4)))""","4")
    t_render("""print(range(-8,-4)[0])""","-8")
    t_render("""print(range(-8,-4)[1])""","-7")
    t_render("""print(range(-8,-4)[-1])""","-5")

    t_render("""print(str(range(-4,-8,-1))[:5])""","<list")
    t_render("""print(len(range(-4,-8,-1)))""","4")
    t_render("""print(range(-4,-8,-1)[0])""","-4")
    t_render("""print(range(-4,-8,-1)[1])""","-5")
    t_render("""print(range(-4,-8,-1)[-1])""","-7")

    t_render("""print(str(range(-4,-8))[:5])""","<list")
    t_render("""print(len(range(-4,-8)))""","0")

    t_render("""print(str(range(-8,-4,-1))[:5])""","<list")
    t_render("""print(len(range(-8,-4,-1)))""","0")

    t_render("""print(str(range(0,4,0))[:5])""","<list")
    t_render("""print(len(range(0,4,0)))""","0")

    
if __name__ == '__main__':
    t_render('print("hello world")',"hello world")
    t_render('print(234)',"234")
    t_render('a=3\nprint(a)',"3")
    t_render('print(2+3)',"5")
    t_render("""
x = 2
x += 3
print(x)
"""
,"5")
    t_render("""
x = "OK"
print(x)
"""
,"OK")
    t_render("""
a,b = 1,2
print(a+b)
"""
,"3")
    t_render("""
x = 1
if x == 1:
    print("yes")
""","yes")
    t_render("""
if 0 == 1:
    print("X")
else:
    print("OK")
"""
,"OK")
    
    t_render("""
if 0 == 1:
    print("X")
elif 1 == 2:
    print("Y")
else:
    print("OK")
"""
,"OK")

    t_render("""
def test(x,y):
    return x+y
r = test(3,5)
print(r)
""","8")
    t_render("""
x = 1
t = 0
while x<=5:
    t = t+x
    x = x+1
print(t)
""","15")
    t_render("""
x = {}
x.y = "test"
print(x.y)
""","test")
    
    t_render("""
if "a" is "a":
    print("OK")
"""
,"OK")

    t_render("""
if "a" is not "b":
    print("OK")
"""
,"OK")

    t_render("""
if None is None:
    print("OK")
"""
,"OK")
    t_render("""
if "x" is "x" or "y" is "y":
    print("OK")
"""
,"OK")
    t_render("""
x = 1
while x < 3:
    break
    x = x + 1
print(x)
"""
,"1")
    t_render("""
x = 1
n = 0
while x < 10:
    x = x + 1
    if n == 2:
        continue
    n = n + 1
print(n)
"""
,"2")
    t_render("""
def test(x): return x
y = test(1)*2 + test(3)*4 + test(5)*6
print(y)
"""
,"44")
    t_render("""
def test(a,b): return a+b
print(test(1,1)+test(1,1))
"""
,"4")

    t_render("""
def test(): print("OK")
x = test
x()
"""
,"OK")

    t_render("""
x = [2,4,6]
print(x[1])
"""
,"4")
    t_render("""
def test(): print("OK")
x = [1,test,2]
x[1]()
"""
,"OK")


    t_render("""
z = 0
for x in [1,2,3]:
    z += x
print(z)
"""
,"6")

    t_render("""
z = 0
for x in range(1,4):
    z += x
print(z)
"""
,"6")

    t_render("""
x = {'a':'OK'}
print(x.a)
"""
,"OK")

    t_render("""print("1234"[1:3])""","23")
    t_render("""print("1234"[-3:3])""","23")
    t_render("""print([1,2,"OK",4][-3:3][1])""","OK")
    t_render("""
n = 0
for x in range(0,10,2):
    n += 1
print(n)
"""
,"5")

    t_render("""print(max(3,8,2,6))""","8")
    t_render("""print(min(3,4,2,6))""","2")
    t_render("""for k in {'OK':0}: print(k)""","OK")

    t_render("""
X = "OK"
def test(): print(X)
test()
"""
,"OK")

    t_render("""
a = 4
def test(z):
    for i in range(0,a):
        z += i
    return z
print(test(1))
"""
,"7")

    t_render("""
def test(self): print(self)
fnc = bind(test,"OK")
fnc()
"""
,"OK")

    t_render("""
class C:
    def __init__(self,data): self.data = data
    def print(self): print(self.data)
C("OK").print()
"""
,"OK")

    t_render("""
x = [v*v for v in range(0,5)]
print(x[3])
"""
,"9")
    t_render("""
t = [[y*10+x for x in range(0,10)] for y in range(0,10)]
print(t[2][3])
"""
,"23")

    t_render("""
x = [1]
x.extend([2,3])
print(x[1])
"""
,"2")

    #t_render("""
#x = {'a':3}
#merge(x,{'b':4})
#print(x.b)
#"""
#,"4")

    t_render("""
x = [1,2,3]
y = copy(x)
y[0] *= 10
print(x[0]+y[0])
"""
,"11")
    t_render("""
x = {'a':3}
y = copy(x)
y.a *= 10
print(x.a+y.a)
"""
,"33")
    t_render("""
x = {}
y = x['x']
"""
,'KeyError',0)
    t_render("""
x = []
y = x[1]
"""
,'KeyError',0)
    t_render("""print("O"+"K")""","OK")
    t_render("""print("-".join(["O","K"]))""","O-K")
    t_render("""print("OK-OK".split("-")[1])""","OK")
    t_render("""
def test(*v): return max(v[2])
print(test(*[1,2,"OK"]))
"""
,"OK")
    t_render("""
def test(**v): return v['x']
print(test(**{'x':'OK'}))
"""
,"OK")
    #t_render("""
#def test(y='K',x='Z'): print(x+y)
#test(x='O')
#"""
#,"OK")
    t_render("""
def test(y='K',x='Z'): print(x+y)
test('O')
"""
,"ZO")

    #t_render("""
#def test(a,b=2,*c,**d): return a+b+c[0]+d['x']+d['z']
#print(test(10,3,z=50000,*[200],**{'x':4000}))
#"""
#,"54213")

    t_render("""print("".join(["O"]+["K"]))""","OK")
    t_render("""x="OK";print(x)""","OK")
    t_render("""x = [1,2,] ; print(x[1])""","2")
    t_render("""a,b,d = [0],0,'OK'; print(d)""","OK")
    
    t_render("""
def test(): raise
try:
    test()
except:
    print("OK")
""","OK")

    t_render("""print("OKx"[:-1])""","OK")
    t_render("""print("xOK"[1:])""","OK")
    t_render("""a,b = "OK"; print(a+b)""","OK")
    
    t_render("""
def test(a,b):
    print a+b[2]
test(1,3)
""","Exception",False)

    t_render("""
def test(): raise
test()
""","Exception",False)
    
    t_render(['OK="OK"',"import tmp1\nprint(tmp1.OK)"],"OK")
    
    t_render(['O="O"','K="K"',"import tmp1, tmp2\nprint(tmp1.O+tmp2.K)"],"OK")
        
    t_render("""
def test(): return
x = 1
print(test())
""","None")

    t_render("""
def test(): pass
x = 1
print(test())
""","None")

    t_render("""
def test():
    global x
    x = "OK"
test()
print(x)
""","OK")

    t_render("""
class X:
    pass
y = X()
print("OK")
""","OK")

    t_render("""
class X: pass
def test(): y = X()
test()
print("OK")
""","OK")

    t_render(["class X: pass\ndef test(): y = X()","import tmp1\ntmp1.test();print('OK')"],"OK")
    
    t_render("print(len([1,2,3]))","3")
    
    t_render('if not "?" in "xyz": print("OK")',"OK")
    
    t_render('print({1:"OK"}[1])',"OK")
    
    t_render('print(len("\0"))',"1")
    
    t_render('print(1 in {1:2})',"1")
    
    t_render('x = {1:2}; del x[1]; print(len(x))','0')
    
    t_render("""
def test(t):
    t = "O"+t
    print(t)
test("K")
""","OK")

    t_render("""print([1,2,3].index(3))""","2")
    t_render("""print("1,2,3".split(",").index("3"))""","2")
    
    t_render("""v = [3,2,1]; v.sort(); print(v[0])""","1")

    t_render("""print(abs(-5))""","5")
    t_render("""print(int(1.234))""","1")
    
    t_render("print(int(round(1.5)))","2")
    t_render("print(ord('X'))","88")
    t_render("print(ord(chr(128)))","128")
    #t_render("print(fsize('LICENSE.txt'))","181")
    t_render("print(int('ff',16))","255")
    t_render("""
def test(x,y): print(x); return y
test('a',1) or test('b',1) and test('c',0)
""","a")

    t_render("def test(): print('OK')\n{'__call__':test}()","OK")

    t_render("""
class A:
    def __init__(self):
        self.a = 'O'
        self.b = 'x'
    def test(self):
        print("KO")
class B(A):
    def __init__(self):
        A.__init__(self)
        self.b = 'K'
    def test(self):
        print(self.a+self.b)
B().test()
""","OK")

    t_render("""
class A:
    def test(self):
        print(self)
A.test("OK")
""","OK")

    t_render("""
def test():
    def fnc():
        print("OK")
    fnc()
test()
""","OK")

    t_render("""print("aa..bbb...ccc".replace("..","X"))""","aaXbbbX.ccc")
    t_render("""print("..bbb..".replace("..","X"))""","XbbbX")
    t_render("""print("234".replace("\r\n","\n"))""","234")
    t_render("""print("a\0b".replace("\0","X"))""","aXb")
    t_render("""x = "a\0b"; x = x.replace("\0","c"); print(x)""","acb")

    t_render("""print(0xff)""","255")
    
    t_render("""x=(1,3);print({x:'OK'}[x])""","OK")
    t_render("""x=(1,3);y=(1,3);print({x:'OK'}[y])""","OK")
    t_render("""print({(1,3):'OK'}[(1,3)])""","OK")
    t_render("def test(): test()\ntest()","Exception",0)
    t_render("x = []; x.append(x); print(x<x)","0");
    t_render("x = []; x.append(x); print({x:'OK'}[x])","OK")
    #t_render("print(float(str(4294967296))==float('4294967296'))","1")
    t_render("print(2**3)","8")
    #t_render("x = 'OK',\nprint(x[0])","OK")

    test_range()
    
    t_render(['v="OK"',"from tmp1 import *\nprint(v)"],"OK")
    t_render(['v="OK"',"from tmp1 import v\nprint(v)"],"OK")
    t_render(['x="X";y="K"',"x = 'O'\nfrom tmp1 import y\nprint(x+y)"],"OK")

    t_render("""
def test(**e):
    print(e['x'])
test(x='OK')
""","OK")

    # test register allocator
    s = "def f():pass\n"+("f()\n"*256)+"print('OK')"
    t_render(s,"OK")
    
    t_render("print(2**3)","8")
    t_render("print(2*3**2)", "18", False)
    
    
    t_render("""
def test(**v): return 'OK'
print(test())
"""
,"OK")
    t_render("""
def test(**v):
    v['x'] = 'OK'
    return v
print(test()['x'])
"""
,"OK")

################################################################################

def t_boot(ss,ex,exact=True):
    if not istype(ss,'list'): ss =[ss]
    n = 1
    for s in ss:
        fname = 'tmp'+str(n)+'.tpc'
        system_rm(fname)
        fname = 'tmp'+str(n)+'.py'
        save(fname,s)
        n += 1
    system_rm('tmp.txt')
    system(TINYPY+fname+' > tmp.txt')
    res = load(TMP).strip()
    #print(ss,ex,res)
    if exact: assert(res == ex)
    else: assert(ex in res)

is_boot = False
try:
    assert(is_tinypy == True)
    x = compile('x=3','')
    is_boot = True
except:
    pass

if is_boot == True and __name__ == '__main__':
    print("# t_boot")
    t_boot(["def test(): print('OK')","import tmp1; tmp1.test()"],"OK")

