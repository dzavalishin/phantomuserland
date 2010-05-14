
spinlock.o:     file format pe-i386


Disassembly of section .text:

00000000 <_hal_min_pagesize>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	b8 00 10 00 00       	mov    $0x1000,%eax
   8:	5d                   	pop    %ebp
   9:	c3                   	ret    

0000000a <_hal_mem_pagesize>:
   a:	55                   	push   %ebp
   b:	89 e5                	mov    %esp,%ebp
   d:	b8 00 10 00 00       	mov    $0x1000,%eax
  12:	5d                   	pop    %ebp
  13:	c3                   	ret    

00000014 <_hal_address_is_aligned>:
  14:	55                   	push   %ebp
  15:	89 e5                	mov    %esp,%ebp
  17:	53                   	push   %ebx
  18:	83 ec 08             	sub    $0x8,%esp
  1b:	8b 5d 08             	mov    0x8(%ebp),%ebx
  1e:	e8 dd ff ff ff       	call   0 <_hal_min_pagesize>
  23:	89 45 f4             	mov    %eax,-0xc(%ebp)
  26:	89 d8                	mov    %ebx,%eax
  28:	ba 00 00 00 00       	mov    $0x0,%edx
  2d:	f7 75 f4             	divl   -0xc(%ebp)
  30:	89 d0                	mov    %edx,%eax
  32:	85 c0                	test   %eax,%eax
  34:	0f 94 c0             	sete   %al
  37:	0f b6 c0             	movzbl %al,%eax
  3a:	83 c4 08             	add    $0x8,%esp
  3d:	5b                   	pop    %ebx
  3e:	5d                   	pop    %ebp
  3f:	c3                   	ret    

00000040 <_read_eflags>:
  40:	55                   	push   %ebp
  41:	89 e5                	mov    %esp,%ebp
  43:	83 ec 10             	sub    $0x10,%esp
  46:	9c                   	pushf  
  47:	58                   	pop    %eax
  48:	89 45 fc             	mov    %eax,-0x4(%ebp)
  4b:	8b 45 fc             	mov    -0x4(%ebp),%eax
  4e:	c9                   	leave  
  4f:	c3                   	ret    

00000050 <_write_eflags>:
  50:	55                   	push   %ebp
  51:	89 e5                	mov    %esp,%ebp
  53:	8b 45 08             	mov    0x8(%ebp),%eax
  56:	50                   	push   %eax
  57:	9d                   	popf   
  58:	5d                   	pop    %ebp
  59:	c3                   	ret    

0000005a <_disable_intr>:
  5a:	55                   	push   %ebp
  5b:	89 e5                	mov    %esp,%ebp
  5d:	fa                   	cli    
  5e:	5d                   	pop    %ebp
  5f:	c3                   	ret    

00000060 <_get_esp>:
  60:	55                   	push   %ebp
  61:	89 e5                	mov    %esp,%ebp
  63:	83 ec 10             	sub    $0x10,%esp
  66:	89 65 fc             	mov    %esp,-0x4(%ebp)
  69:	8b 45 fc             	mov    -0x4(%ebp),%eax
  6c:	c9                   	leave  
  6d:	c3                   	ret    

0000006e <_get_ebp>:
  6e:	55                   	push   %ebp
  6f:	89 e5                	mov    %esp,%ebp
  71:	83 ec 10             	sub    $0x10,%esp
  74:	89 6d fc             	mov    %ebp,-0x4(%ebp)
  77:	8b 45 fc             	mov    -0x4(%ebp),%eax
  7a:	c9                   	leave  
  7b:	c3                   	ret    

0000007c <_get_ss>:
  7c:	55                   	push   %ebp
  7d:	89 e5                	mov    %esp,%ebp
  7f:	83 ec 10             	sub    $0x10,%esp
  82:	8c 55 fc             	mov    %ss,-0x4(%ebp)
  85:	8b 45 fc             	mov    -0x4(%ebp),%eax
  88:	c9                   	leave  
  89:	c3                   	ret    

0000008a <_rdmsr>:
  8a:	55                   	push   %ebp
  8b:	89 e5                	mov    %esp,%ebp
  8d:	83 ec 10             	sub    $0x10,%esp
  90:	8b 4d 08             	mov    0x8(%ebp),%ecx
  93:	0f 32                	rdmsr  
  95:	89 45 f8             	mov    %eax,-0x8(%ebp)
  98:	89 55 fc             	mov    %edx,-0x4(%ebp)
  9b:	8b 45 f8             	mov    -0x8(%ebp),%eax
  9e:	8b 55 fc             	mov    -0x4(%ebp),%edx
  a1:	c9                   	leave  
  a2:	c3                   	ret    

000000a3 <_wrmsr>:
  a3:	55                   	push   %ebp
  a4:	89 e5                	mov    %esp,%ebp
  a6:	83 ec 08             	sub    $0x8,%esp
  a9:	8b 45 0c             	mov    0xc(%ebp),%eax
  ac:	89 45 f8             	mov    %eax,-0x8(%ebp)
  af:	8b 45 10             	mov    0x10(%ebp),%eax
  b2:	89 45 fc             	mov    %eax,-0x4(%ebp)
  b5:	8b 45 f8             	mov    -0x8(%ebp),%eax
  b8:	8b 55 fc             	mov    -0x4(%ebp),%edx
  bb:	8b 4d 08             	mov    0x8(%ebp),%ecx
  be:	0f 30                	wrmsr  
  c0:	c9                   	leave  
  c1:	c3                   	ret    

000000c2 <_rdldt>:
  c2:	55                   	push   %ebp
  c3:	89 e5                	mov    %esp,%ebp
  c5:	83 ec 10             	sub    $0x10,%esp
  c8:	0f 00 45 fe          	sldt   -0x2(%ebp)
  cc:	0f b7 45 fe          	movzwl -0x2(%ebp),%eax
  d0:	c9                   	leave  
  d1:	c3                   	ret    

000000d2 <_wrldt>:
  d2:	55                   	push   %ebp
  d3:	89 e5                	mov    %esp,%ebp
  d5:	83 ec 04             	sub    $0x4,%esp
  d8:	8b 45 08             	mov    0x8(%ebp),%eax
  db:	66 89 45 fc          	mov    %ax,-0x4(%ebp)
  df:	0f b7 45 fc          	movzwl -0x4(%ebp),%eax
  e3:	0f 00 d0             	lldt   %ax
  e6:	c9                   	leave  
  e7:	c3                   	ret    

000000e8 <_hal_spin_init>:
  e8:	55                   	push   %ebp
  e9:	89 e5                	mov    %esp,%ebp
  eb:	8b 45 08             	mov    0x8(%ebp),%eax
  ee:	c7 00 00 00 00 00    	movl   $0x0,(%eax)
  f4:	8b 45 08             	mov    0x8(%ebp),%eax
  f7:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
  fe:	5d                   	pop    %ebp
  ff:	c3                   	ret    

00000100 <_hal_spin_lock>:
 100:	55                   	push   %ebp
 101:	89 e5                	mov    %esp,%ebp
 103:	83 ec 08             	sub    $0x8,%esp
 106:	e8 00 00 00 00       	call   10b <_hal_spin_lock+0xb>
 10b:	85 c0                	test   %eax,%eax
 10d:	74 0c                	je     11b <_hal_spin_lock+0x1b>
 10f:	c7 04 24 00 00 00 00 	movl   $0x0,(%esp)
 116:	e8 00 00 00 00       	call   11b <_hal_spin_lock+0x1b>
 11b:	8b 45 08             	mov    0x8(%ebp),%eax
 11e:	8b 00                	mov    (%eax),%eax
 120:	85 c0                	test   %eax,%eax
 122:	74 2f                	je     153 <_hal_spin_lock+0x53>
 124:	c7 04 24 14 00 00 00 	movl   $0x14,(%esp)
 12b:	e8 00 00 00 00       	call   130 <_hal_spin_lock+0x30>
 130:	8b 45 08             	mov    0x8(%ebp),%eax
 133:	8b 40 04             	mov    0x4(%eax),%eax
 136:	89 04 24             	mov    %eax,(%esp)
 139:	e8 00 00 00 00       	call   13e <_hal_spin_lock+0x3e>
 13e:	c7 04 24 45 00 00 00 	movl   $0x45,(%esp)
 145:	e8 00 00 00 00       	call   14a <_hal_spin_lock+0x4a>
 14a:	8b 45 08             	mov    0x8(%ebp),%eax
 14d:	8b 00                	mov    (%eax),%eax
 14f:	85 c0                	test   %eax,%eax
 151:	75 f7                	jne    14a <_hal_spin_lock+0x4a>
 153:	8b 45 08             	mov    0x8(%ebp),%eax
 156:	ba 01 00 00 00       	mov    $0x1,%edx
 15b:	87 10                	xchg   %edx,(%eax)
 15d:	89 d0                	mov    %edx,%eax
 15f:	85 c0                	test   %eax,%eax
 161:	75 e7                	jne    14a <_hal_spin_lock+0x4a>
 163:	e8 06 ff ff ff       	call   6e <_get_ebp>
 168:	89 c2                	mov    %eax,%edx
 16a:	8b 45 08             	mov    0x8(%ebp),%eax
 16d:	89 50 04             	mov    %edx,0x4(%eax)
 170:	c9                   	leave  
 171:	c3                   	ret    

00000172 <_hal_spin_unlock>:
 172:	55                   	push   %ebp
 173:	89 e5                	mov    %esp,%ebp
 175:	83 ec 08             	sub    $0x8,%esp
 178:	8b 45 08             	mov    0x8(%ebp),%eax
 17b:	c7 40 04 00 00 00 00 	movl   $0x0,0x4(%eax)
 182:	8b 45 08             	mov    0x8(%ebp),%eax
 185:	31 d2                	xor    %edx,%edx
 187:	87 10                	xchg   %edx,(%eax)
 189:	e8 00 00 00 00       	call   18e <_hal_spin_unlock+0x1c>
 18e:	85 c0                	test   %eax,%eax
 190:	74 0c                	je     19e <_hal_spin_unlock+0x2c>
 192:	c7 04 24 4d 00 00 00 	movl   $0x4d,(%esp)
 199:	e8 00 00 00 00       	call   19e <_hal_spin_unlock+0x2c>
 19e:	c9                   	leave  
 19f:	c3                   	ret    
