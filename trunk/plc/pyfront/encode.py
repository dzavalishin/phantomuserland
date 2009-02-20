import tokenize
from tokenize import Token
if '.' in str(1.0):
    from boot import *

#EOF,ADD,SUB,MUL,DIV,POW,AND,OR,CMP,GET,SET,NUMBER,STRING,GGET,GSET,MOVE,DEF,PASS,JUMP,CALL,RETURN,IF,DEBUG,EQ,LE,LT,DICT,LIST,NONE,LEN,POS,PARAMS,IGET,FILE,NAME,NE,HAS,RAISE,SETJMP,MOD,LSH,RSH,ITER,DEL,REGS = 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44

class DState:
    def __init__(self,code,fname):
        self.code, self.fname = code,fname
        self.lines = self.code.split('\n')

        self.stack,self.out,self._scopei,self.tstack,self._tagi,self.data = [],[('tag','EOF')],0,[],0,{}
        self.error = False
    def begin(self,gbl=False):
        if len(self.stack): self.stack.append((self.vars,self.r2n,self.n2r,self._tmpi,self.mreg,self.snum,self._globals,self.lineno,self.globals,self.cregs,self.tmpc))
        else: self.stack.append(None)
        self.vars,self.r2n,self.n2r,self._tmpi,self.mreg,self.snum,self._globals,self.lineno,self.globals,self.cregs,self.tmpc = [],{},{},0,0,str(self._scopei),gbl,-1,[],['regs'],0
        self._scopei += 1
        code('<PythonCode>')
        insert(self.cregs)
    def end(self):
        self.cregs.append(self.mreg)
        xml('EOF')
        code('</PythonCode>')
        
        # This next line forces the encoder to
        # throw an exception if any tmp regs 
        # were leaked within the frame
        assert(self.tmpc == 0) #REG
        
        if len(self.stack) > 1:
            self.vars,self.r2n,self.n2r,self._tmpi,self.mreg,self.snum,self._globals,self.lineno,self.globals,self.cregs,self.tmpc = self.stack.pop()
        else: self.stack.pop()


def insert(v):
    D.out.append(v)
    #D.out.append(('---'))
def write(v):
    if istype(v,'list'):
        insert(v)
        return
    #for n in range(0,len(v),4):
    #    insert(('data',v[n:n+4]))
    insert(('data',v))
def setpos(v):
    if '-nopos' in ARGV: return
    #return
    line,x = v
    if line == D.lineno: return
    text = D.lines[line-1]
    D.lineno = line
    val = text + "\0"*(4-len(text)%4)
    #code('POS','strlen='+str(len(val)),'line='+str(line), "'"+val+"'")
    #code('<POS','line='+str(line), 'text="'+text+'"',' />')
    xml('pos','line',line, 'text',text)
    #comment('Src line='+str(line)+"'"+text+"'")
    #write(val)
def code(i,a='',b='',c=''):
    #if not istype(i,'number'): raise
    #if not istype(a,'number'): raise
    #if not istype(b,'number'): raise
    #if not istype(c,'number'): raise
    write(('code',str(i),str(a),str(b),str(c)))
def xml(tag, p1='', v1='', p2='', v2='', p3='', v3=''):
    parms = ''
    if( p1 != '' ):
        parms += ' '+p1+'="'+str(v1)+'"'
    if( p2 != '' ):
        parms += ' '+p2+'="'+str(v2)+'"'
    if( p3 != '' ):
        parms += ' '+p3+'="'+str(v3)+'"'

    write(('code','<'+tag+parms+' />', '', '', ''))

def emitmove(to,frm):
    xml('MOVE','to',to,'from', frm)

def emitgget(to,varname):
    xml('GGET', 'to', to, 'gVarName', varname)

def emitgset(varname,frr):
    xml('GSET', 'gVarName', varname, 'fromreg', frr)


def emitset(kls,fld,reg):
    xml('SET',"class",kls,"fieldName", fld,"fromreg",reg)

def emitget(toreg,cls,fld):
    xml('GET','toreg',toreg,'class',cls,"fieldName",fld)

def emiteq(to,a,b):
    xml('EQ','result',to,'left',a,'right',b)

def emitif(reg):
    xml('IF','reg',reg)


def comment(text):
    write(('code','<!--',text,'','-->'))
def code_16(i,a,b):
    if b < 0: b += 0x8000
    #code(i,a,(b&0xff00)>>8,(b&0xff)>>0)
    code(i,a,b,'')
def get_code16(i,a,b):
    #return ('code',i,a,(b&0xff00)>>8,(b&0xff)>>0)
    return ('code',i,a,b,'')

def _do_string(v,r=None):
    r = get_tmp(r)
    #val = v + "\0"*(4-len(v)%4)
    #code_16('STRING',r,len(v))
    #write(val)
    code('<STRING','reg="'+str(r)+'" content="'+str(v)+'"', 'len="'+str(len(v))+'"', ' />' )
    return r
def do_string(t,r=None):
    return _do_string(t.val,r)

def _do_number(v,r=None):
    r = get_tmp(r)
    #code('NUMBER',r,0,0)
    #write(fpack(number(v)))
    #code('NUMBER','reg='+str(r),'val='+str(fpack(number(v))))
    code('<NUMBER','reg="'+str(r)+'" val="'+str(number(v))+'"', ' />' )
    return r
def do_number(t,r=None):
    return _do_number(t.val,r)

def get_tag():
    k = str(D._tagi)
    D._tagi += 1
    return k
def stack_tag():
    k = get_tag()
    D.tstack.append(k)
    return k
def pop_tag():
    D.tstack.pop()

def tag(*t):
    t = D.snum+':'+':'.join([str(v) for v in t])
    insert(('tag',t))
def jump(*t):
    t = D.snum+':'+':'.join([str(v) for v in t])
    insert(('jump',t))
def setjmp(*t):
    t = D.snum+':'+':'.join([str(v) for v in t])
    insert(('setjmp',t))
def fnc(*t):
    t = D.snum+':'+':'.join([str(v) for v in t])
    r = get_reg(t)
    insert(('fnc',r,t))
    return r

def map_tags():
    tags = {}
    out = []
    n = 0
    for item in D.out:
        if item[0] == 'tag':
            tags[item[1]] = n
            out.append(item)
            continue
        if item[0] == 'regs':
            out.append(get_code16('<REGS num="'+str(item[1])+'" />','',''))
            n += 1
            continue
        out.append(item)
        n += 1
    for n in range(0,len(out)):
        item = out[n]
        if item[0] == 'tag':
            out[n] = ('code', '<Label name="'+str(item[1])+'" />', '', '', '')
        elif item[0] == 'jump':
            #out[n] = get_code16('<JUMP a="'+str(item[1])+'" b="'+str(tags[item[1]]-n)+'" />','','')
            out[n] = get_code16('<JUMP label="'+str(item[1])+'" />','','')
        elif item[0] == 'setjmp':
            out[n] = get_code16('SETJMP',item[1],tags[item[1]]-n)
        elif item[0] == 'fnc':
            #out[n] = get_code16('DefFunc',item[1],tags[item[2]]-n)
            out[n] = ('code', '<DefFunc outreg="'+str(item[1])+'" len="'+str(tags[item[2]]-n)+'" />', '', '', '')

    for n in range(0,len(out)):
        item = out[n]
        if item[0] == 'data':
            out[n] = "string '"+item[1] +"'\n\n"
            #out[n] = str(out[n]) +'\n'
        elif item[0] == 'code':
            i,a,b,c = item[1:]
            #out[n] = chr(i)+chr(a)+chr(b)+chr(c)
            out[n] = str(i)+' '+str(a)+' '+str(b)+' '+str(c)+'\n'
        else:
            raise str(('huh?',item))
        #if len(out[n]) != 4:
        #    raise ('code '+str(n)+' is wrong length '+str(len(out[n])))
    D.out = out

def get_tmp(r=None):
    if r != None: return r
    return get_tmps(1)[0]
def get_tmps(t):
    rs = alloc(t)
    regs = range(rs,rs+t)
    for r in regs:
        set_reg(r,"$"+str(D._tmpi))
        D._tmpi += 1
    D.tmpc += t #REG
    return regs
def alloc(t):
    s = ''.join(["01"[r in D.r2n] for r in range(0,min(256,D.mreg+t))])
    return s.index('0'*t)
def is_tmp(r):
    if r is None: return False
    return (D.r2n[r][0] == '$')
def un_tmp(r):
    n = D.r2n[r]
    free_reg(r)
    set_reg(r,'*'+n)
def free_tmp(r):
    if is_tmp(r): free_reg(r)
    return r
def free_tmps(r):
    for k in r: free_tmp(k)
def get_reg(n):
    if n not in D.n2r:
        set_reg(alloc(1),n)
    return D.n2r[n]
#def get_clean_reg(n):
    #if n in D.n2r: raise
    #set_reg(D.mreg,n)
    #return D.n2r[n]
def set_reg(r,n):
    D.n2r[n] = r; D.r2n[r] = n
    D.mreg = max(D.mreg,r+1)
def free_reg(r):
    if is_tmp(r): D.tmpc -= 1
    n = D.r2n[r]; del D.r2n[r]; del D.n2r[n]

def imanage(orig,fnc):
    items = orig.items
    orig.val = orig.val[:-1]
    t = Token(orig.pos,'symbol','=',[items[0],orig])
    return fnc(t)

def infix(i,tb,tc,r=None):
    r = get_tmp(r)
    b,c = do(tb,r),do(tc)
    #code(i,'out='+str(r),b,c)
    xml(i,'out',r,'left',b,'right',c)
    if r != b: free_tmp(b)
    free_tmp(c)
    return r
def ss_infix(ss,i,tb,tc,r=None):
    r = get_tmp(r)
    r2 = get_tmp()
    ss = _do_number(ss)
    t = get_tag()
    r = do(tb,r)
    #code('EQ',r2,r,ss)
    #xml('EQ','result',r2,'left',r,'right',ss)
    emiteq(r2,r,ss)
    #code('IF',r2)
    #xml('IF','reg',r2)
    emitif(r2)
    jump(t,'else')
    jump(t,'end')
    tag(t,'else')
    r = do(tc,r)
    tag(t,'end')
    free_tmp(r2) #REG
    free_tmp(ss) #REG
    return r

def _do_none(r=None):
    r = get_tmp(r)
    code('<NONE reg="'+str(r)+'" /><!-- set reg '+str(r)+' to null -->')
    return r

def do_symbol(t,r=None):
    sets = ['=']
    isets = ['+=','-=','*=','/=']
    cmps = ['<','>','<=','>=','==','!=']
    metas = {
        '+':'ADD','*':'MUL','/':'DIV','**':'POW',
        '-':'SUB','and':'AND','or':'OR',
        '%':'MOD','>>':'RSH','<<':'LSH',
        '&':'AND','|':'OR',
    }
    if t.val == 'None': return _do_none(r)
    if t.val == 'True':
        return _do_number('1',r)
    if t.val == 'False':
        return _do_number('0',r)
    items = t.items

    if t.val in ['and','or']:
        ss = int(t.val == 'or')
        return ss_infix(ss,metas[t.val],items[0],items[1],r)
    if t.val in isets:
        return imanage(t,do_symbol)
    if t.val == 'is':
        return infix('EQ',items[0],items[1],r)
    if t.val == 'isnot':
        return infix('CMP',items[0],items[1],r)
    if t.val == 'not':
        return infix('EQ',Token(t.pos,'number',0),items[0],r)
    if t.val == 'in':
        return infix('HAS',items[1],items[0],r)
    if t.val == 'notin':
        r = infix('HAS',items[1],items[0],r)
        zero = _do_number('0')
        #code('EQ',r,r,free_tmp(zero))
        emiteq(r,r,free_tmp(zero))
        return r
    if t.val in sets:
        return do_set_ctx(items[0],items[1]);
    elif t.val in cmps:
        b,c = items[0],items[1]
        v = t.val
        if v[0] in ('>','>='):
            b,c,v = c,b,'<'+v[1:]
        cd = 'EQ'
        if v == '<': cd = LT
        if v == '<=': cd = LE
        if v == '!=': cd = NE
        return infix(cd,b,c,r)
    else:
        return infix(metas[t.val],items[0],items[1],r)

def do_set_ctx(k,v):
    if k.type == 'name':
        if (D._globals and k.val not in D.vars) or (k.val in D.globals):
            #c = do_string(k)
            b = do(v)
            #code('<GSET gVarName="'+k.val+'" reg="'+str(b)+'" />','','','')
            emitgset(k.val,b)
            #free_tmp(c)
            free_tmp(b)
            return
        a = do_local(k)
        b = do(v)
        #code('<MOVE to="'+str(a)+'" from="'+str(b)+'" />', '', '', '')
        emitmove(a,b)
        free_tmp(b)
        return a
    elif k.type in ('tuple','list'):
        if v.type in ('tuple','list'):
            n,tmps = 0,[]
            for kk in k.items:
                vv = v.items[n]
                tmp = get_tmp(); tmps.append(tmp)
                r = do(vv)
                #code('< MOVE to="'+str(tmp)+'" from="'+str(r)+'" />', '', '', '')
                emitmove(tmp,r)
                free_tmp(r) #REG
                n+=1
            n = 0
            for kk in k.items:
                vv = v.items[n]
                tmp = tmps[n]
                free_tmp(do_set_ctx(kk,Token(vv.pos,'reg',tmp))) #REG
                n += 1
            return

        r = do(v); un_tmp(r)
        n, tmp = 0, Token(v.pos,'reg',r)
        for tt in k.items:
            free_tmp(do_set_ctx(tt,Token(tmp.pos,'get',None,[tmp,Token(tmp.pos,'number',str(n))]))) #REG
            n += 1
        free_reg(r)
        return
    r = do(k.items[0])
    rr = do(v)
    tmp = do(k.items[1])
    #code('SET',r,tmp,rr)
    emitset(r,tmp,rr)
    free_tmp(r) #REG
    free_tmp(tmp) #REG
    return rr

def manage_seq(i,a,items,sav=0):
    l = max(sav,len(items))
    n,tmps = 0,get_tmps(l)
    for tt in items:
        r = tmps[n]
        b = do(tt,r)
        if r != b:
            #code('MOVE','to='+str(r),'from='+str(b))
            emitmove(r,b)
            free_tmp(b)
        #print 'seq out to reg '+str(r)
        n +=1
    if not len(tmps):
        xml(i,"out",a,"inStart",0,"inNum",0)
        #code(i,a,0,0)
        return 0
    #code(i,a,tmps[0],len(items))
    xml(i,"out",a,"inStart",tmps[0],"inNum",len(items))
    free_tmps(tmps[sav:])
    return tmps[0]

def p_filter(items):
    a,b,c,d = [],[],None,None
    for t in items:
        if t.type == 'symbol' and t.val == '=': b.append(t)
        elif t.type == 'args': c = t
        elif t.type == 'nargs': d = t
        else: a.append(t)
    return a,b,c,d

def do_import(t):
    for mod in t.items:
        mod.type = 'string'
        v = do_call(Token(t.pos,'call',None,[
            Token(t.pos,'name','import'),
            mod]))
        mod.type = 'name'
        do_set_ctx(mod,Token(t.pos,'reg',v))
def do_from(t):
    mod = t.items[0]
    mod.type = 'string'
    v = do(Token(t.pos,'call',None,[
        Token(t.pos,'name','import'),
        mod]))
    item = t.items[1]
    if item.val == '*':
        free_tmp(do(Token(t.pos,'call',None,[
            Token(t.pos,'name','merge'),
            Token(t.pos,'name','__dict__'),
            Token(t.pos,'reg',v)]))) #REG
    else:
        item.type = 'string'
        free_tmp(do_set_ctx(
            Token(t.pos,'get',None,[ Token(t.pos,'name','__dict__'),item]),
            Token(t.pos,'get',None,[ Token(t.pos,'reg',v),item])
            )) #REG

        
def do_globals(t):
    for t in t.items:
        if t.val not in D.globals:
            D.globals.append(t.val)
def do_del(tt):
    for t in tt.items:
        r = do(t.items[0])
        r2 = do(t.items[1])
        code('DEL',r,r2)
        free_tmp(r); free_tmp(r2) #REG

def do_call(t,r=None):
    r = get_tmp(r)
    items = t.items
    fnc = do(items[0])
    a,b,c,d = p_filter(t.items[1:])
    e = None
    if len(b) != 0 or d != None:
        e = do(Token(t.pos,'dict',None,[])); un_tmp(e);
        for p in b:
            p.items[0].type = 'string'
            t1,t2 = do(p.items[0]),do(p.items[1])
            #code('SET',e,t1,t2)
            emitset(e,t1,t2)
            free_tmp(t1); free_tmp(t2) #REG
        if d: free_tmp(do(Token(t.pos,'call',None,[Token(t.pos,'name','merge'),Token(t.pos,'reg',e),d.items[0]]))) #REG
    manage_seq('PARAMS',r,a)
    if c != None:
        t1,t2 = _do_string('*'),do(c.items[0])
        #code('SET',r,t1,t2)
        emitset(r,t1,t2)
        free_tmp(t1); free_tmp(t2) #REG
    if e != None:
        t1 = _do_none()
        #code('SET',r,t1,e)
        emitset(r,t1,e)
        free_tmp(t1) #REG
    xml('CALL','ret',r,'func',fnc,'params',r)
    free_tmp(fnc) #REG
    return r

def do_name(t,r=None):
    if t.val in D.vars:
        return do_local(t,r)
    r = get_tmp(r)
    #c = do_string(t)
    #code('GGET','reg='+str(r),c)
    #free_tmp(c)
    #code('<GGET reg="'+str(r)+'" gVarName="'+t.val+'" />', '', '', '')
    emitgget( r, t.val );
    return r

def do_local(t,r=None):
    if t.val not in D.vars:
        D.vars.append(t.val)
    return get_reg(t.val)

def do_def(tok,kls=None):
    items = tok.items

    t = get_tag()
    rf = fnc(t,'end')

    D.begin()
    setpos(tok.pos)
    r = do_local(Token(tok.pos,'name','__params'))
    do_info(items[0].val)
    a,b,c,d = p_filter(items[1].items)
    for p in a:
        v = do_local(p)
        tmp = _do_none()
        #code('GET','toreg='+str(v),r,tmp)
        emitget(v,r,tmp)
        free_tmp(tmp) #REG
    for p in b:
        v = do_local(p.items[0])
        do(p.items[1],v)
        tmp = _do_none()
        code('IGET',v,r,tmp)
        free_tmp(tmp) #REG
    if c != None:
        v = do_local(c.items[0])
        tmp = _do_string('*')
        #code('GET','toreg='+str(v),r,tmp)
        emitget(v,r,tmp)
        free_tmp(tmp) #REG
    if d != None:
        e = do_local(d.items[0])
        #code('DICT',e,0,0)
        xml('DICT',"reg",e)
        tmp = _do_none()
        code('IGET',e,r,tmp)
        free_tmp(tmp) #REG
    free_tmp(do(items[2])) #REG
    D.end()

    tag(t,'end')

    if kls == None:
        if D._globals: do_globals(Token(tok.pos,0,0,[items[0]]))
        r = do_set_ctx(items[0],Token(tok.pos,'reg',rf))
    else:
        rn = do_string(items[0])
        #code('SET',kls,rn,rf)
        emitset(kls,rn,rf)
        free_tmp(rn)

    free_tmp(rf)

def do_class(t):
    tok = t
    items = t.items
    parent = None
    if items[0].type == 'name':
        name = items[0].val
    else:
        name = items[0].items[0].val
        parent = items[0].items[1].val

    kls = do(Token(t.pos,'dict',0,[]))
    #ts = _do_string(name)
    #code('GSET',ts,kls)
    #free_tmp(ts) #REG

    #code('GSET','gVarName='+name,'reg='+str(kls))
    #code('GSET','gVarName',name,'reg',kls)
    emitgset(name,kls)

    init,_new = False,[]
    if parent:
        _new.append(Token(t.pos,'call',None,[
            Token(t.pos,'get',None,[
                Token(t.pos,'name',parent),
                Token(t.pos,'string','__new__'),
                ]),
            Token(t.pos,'name','self'),
            ]))

    for fc in items[1].items:
        if fc.type != 'def': continue
        fn = fc.items[0].val
        if fn == '__init__': init = True
        do_def(fc,kls)
        _new.append(Token(fc.pos,'symbol','=',[
            Token(fc.pos,'get',None,[
                Token(fc.pos,'name','self'),
                Token(fc.pos,'string',fn)]),
            Token(fc.pos,'call',None,[
                Token(fc.pos,'name','bind'),
                Token(fc.pos,'get',None,[
                    Token(fc.pos,'name',name),
                    Token(fc.pos,'string',fn)]),
                Token(fc.pos,'name','self')])
            ]))

    do_def(Token(t.pos,'def',None,[
        Token(t.pos,'name','__new__'),
        Token(t.pos,'list',None,[Token(t.pos,'name','self')]),
        Token(t.pos,'statements',None,_new)]),kls)

    t = get_tag()
    rf = fnc(t,'end')
    D.begin()
    params = do_local(Token(tok.pos,'name','__params'))

    slf = do_local(Token(tok.pos,'name','self'))
    comment('self');
    #code('DICT',slf,0,0)
    xml('DICT',"reg",slf)

    free_tmp(do(Token(tok.pos,'call',None,[
        Token(tok.pos,'get',None,[
            Token(tok.pos,'name',name),
            Token(tok.pos,'string','__new__')]),
        Token(tok.pos,'name','self')]))) #REG

    if init:
        tmp = get_tmp()
        #t3 = _do_string('__init__')
        #code('GET',tmp,slf,t3)
        #code('GET','toreg='+str(tmp),'fromClass='+str(slf),"varName='__init__'")
        #xml('GET','toreg',tmp,'fromClass',slf,"varName",'__init__')
        emitget(tmp,slf,'__init__')
        t4 = get_tmp()
        #code('CALL',t4,'func='+str(tmp),'params='+str(params))
        xml('CALL','ret',t4,'func',tmp,'params',params)
        free_tmp(tmp) #REG
        #free_tmp(t3) #REG
        free_tmp(t4) #REG
    comment('return self');
    code('<RETURN reg="'+str(slf)+'" />',)

    D.end()
    tag(t,'end')
    #ts = _do_string('__call__')
    #code('SET',kls,ts,rf)
    #code('SET',"class="+str(kls),"fieldName='__call__'",rf)
    #xml('SET',"class",kls,"fieldName", '__call__',"rf",rf)
    emitset(kls,'__call__',rf)
    free_tmp(kls) #REG
    #free_tmp(ts) #REG




def do_while(t):
    items = t.items
    t = stack_tag()
    tag(t,'begin')
    tag(t,'continue')
    r = do(items[0])
    #code('IF',r)
    emitif(r)
    free_tmp(r) #REG
    jump(t,'end')
    free_tmp(do(items[1])) #REG
    jump(t,'begin')
    tag(t,'break')
    tag(t,'end')
    pop_tag()

def do_for(tok):
    items = tok.items

    reg = do_local(items[0])
    itr = do(items[1])
    i = _do_number('0')

    t = stack_tag(); tag(t,'loop'); tag(t,'continue')
    code('ITER',reg,itr,i); jump(t,'end')
    free_tmp(do(items[2])) #REG
    jump(t,'loop')
    tag(t,'break'); tag(t,'end'); pop_tag()

    free_tmp(itr) #REG
    free_tmp(i)

def do_comp(t,r=None):
    name = 'comp:'+get_tag()
    r = do_local(Token(t.pos,'name',name))
    code('LIST',r,0,0)
    key = Token(t.pos,'get',None,[
            Token(t.pos,'reg',r),
            Token(t.pos,'symbol','None')])
    ap = Token(t.pos,'symbol','=',[key,t.items[0]])
    do(Token(t.pos,'for',None,[t.items[1],t.items[2],ap]))
    return r

def do_if(t):
    items = t.items
    t = get_tag()
    n = 0
    for tt in items:
        tag(t,n)
        if tt.type == 'elif':
            a = do(tt.items[0]);
            #code('IF',a);
            emitif(a);
            free_tmp(a);
            jump(t,n+1)
            free_tmp(do(tt.items[1])) #REG
        elif tt.type == 'else':
            free_tmp(do(tt.items[0])) #REG
        else:
            raise
        jump(t,'end')
        n += 1
    tag(t,n)
    tag(t,'end')

def do_try(t):
    items = t.items
    t = get_tag()
    setjmp(t,'except')
    free_tmp(do(items[0])) #REG
    jump(t,'end')
    tag(t,'except')
    free_tmp(do(items[1].items[1])) #REG
    tag(t,'end')

def do_return(t):
    if t.items: r = do(t.items[0])
    else: r = _do_none()
    #code('<RETURN ','reg="'+str(r)+'"','/>')
    xml('RETURN','reg',r)
    free_tmp(r)
    return
def do_raise(t):
    if t.items: r = do(t.items[0])
    else: r = _do_none()
    #code('RAISE',r)
    xml('RAISE','reg',r)
    free_tmp(r)
    return

def do_statements(t):
    for tt in t.items: free_tmp(do(tt))

def do_list(t,r=None):
    r = get_tmp(r)
    manage_seq('LIST',r,t.items)
    return r

def do_dict(t,r=None):
    r = get_tmp(r)
    manage_seq('DICT',r,t.items)
    return r

def do_get(t,r=None):
    items = t.items
    return infix('GET',items[0],items[1],r)

def do_break(t): jump(D.tstack[-1],'break')
def do_continue(t): jump(D.tstack[-1],'continue')
def do_pass(t): xml('PASS')

def do_info(name='?'):
    if '-nopos' in ARGV: return
    #code('FILE',free_tmp(_do_string(D.fname)))
    #code('NAME',free_tmp(_do_string(name)))
    #code('<FILE name="'+D.fname+'" />', '', '', '')
    xml('file', 'name', D.fname)
    #code('<Function name="'+name+'" />')
    xml('Function', 'name', name)
def do_module(t):
    do_info()
    free_tmp(do(t.items[0])) #REG
def do_reg(t,r=None): return t.val

fmap = {
    'module':do_module,'statements':do_statements,'def':do_def,
    'return':do_return,'while':do_while,'if':do_if,
    'break':do_break,'pass':do_pass,'continue':do_continue,'for':do_for,
    'class':do_class,'raise':do_raise,'try':do_try,'import':do_import,
    'globals':do_globals,'del':do_del,'from':do_from,
}
rmap = {
    'list':do_list, 'tuple':do_list, 'dict':do_dict, 'slice':do_list,
    'comp':do_comp, 'name':do_name,'symbol':do_symbol,'number':do_number,
    'string':do_string,'get':do_get, 'call':do_call, 'reg':do_reg,
}

def do(t,r=None):
    if t.pos: setpos(t.pos)
    try:
        if t.type in rmap:
            return rmap[t.type](t,r)
        return fmap[t.type](t)
    except:
        if D.error: raise
        D.error = True
        tokenize.u_error('encode',D.code,t.pos)

def encode(fname,s,t):
    t = Token((1,1),'module','module',[t])
    global D
    s = tokenize.clean(s)
    D = DState(s,fname)
    D.begin(True)
    do(t)
    D.end()
    map_tags()
    out = D.out; D = None
    return ''.join(out)

