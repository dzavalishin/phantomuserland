В процессе работы над ОС Фантом, которая вообще не Юникс никаким местом, мне, тем не менее, захотелось сделать в нём Unix-compatible подсистему. Не то, чтобы прямо POSIX, но что-то достаточно близкое. Отчасти из любопытства, отчасти для удобства, отчасти как ещё один migration path. (Ну и вообще было интересно, насколько трудно написать простенький Юникс "из головы".) В качестве цели номер 1 была поставлена задача запустить quake 1 for Unix, которая и была достигнута.

В процессе, естественно, появились open/close/r/w/ioctl, и появилось ощущение, что последний неприлично, постыдно устарел. В качестве упражнения для размятия мозга я реализовал (в дополнение к обычному ioctl) некоторый альтернативный API, который бы позволил управлять свойствами устройств более гибким и удобным с точки зрения пользователя способом. Этот API, конечно, имеет свои очевидны минусы, и, в целом, эта статья - RFC, aka request For Comments.

Итак, API на уровне пользователя:

<source lang="cpp">
// returns name of property with sequential number nProperty, or error
errno_t listproperties( int fd, int nProperty, char *buf, int buflen );

errno_t getproperty( int fd, const char *pName, char *buf, int buflen );
errno_t setproperty( int fd, const char *pName, const char *pValue );
</source>

Правила:

<ol>
	<li>Никаких дефайнов с номерами, только имена.</li>
	<li>Никаких бинарных данных, только строки</li>
</ol>

Это не очень эффективно, но хочется предположить, что установка/чтение свойств - процесс редкий, и потому упираться в его скорость смысла немного, да и само переключение контекста при вызове стоит немало.

Можно несколько оптимизировать интерфейс, например, так:

<source lang="cpp">
// returns name of property with sequential number nProperty, or error
errno_t listproperties( int fd, int nProperty, char *buf, int buflen );

// returns property id by name
errno_t property_by_name( int fd, int *nProperty, const char *name );


errno_t getproperty( int fd, int nProperty, char *buf, int buflen );
errno_t setproperty( int fd, int nProperty, const char *pValue );

// fast lane

errno_t getproperty_i32( int fd, int nProperty, int32_t *data );
errno_t setproperty_i32( int fd, int nProperty, int32_t data );
</source>

Эта схема для единичного свойства не медленнее, чем ioctl. 

Чем она хороша: можно сделать общую команду (напр mode), которая управляет параметрами любого драйвера, не зная о нём ничего - mode /dev/myCrazyDriver выдаст список свойств, а mode /dev/myCrazyDriver name val установит свойство name в значение val.

Реализация внутри драйвера (для которой, конечно, в ядре есть соответствующая незамысловатая инфраструктура) тоже несложна:

<source lang="cpp">
static property_t proplist[] =
{
    { pt_int32, "leds", 0, &leds, 0, (void *)set_leds, 0, 0 },
};
</source>

Эта строка описывает свойство, которое имеет тип int32, лежит в переменной leds, и если оно изменилось, то надобно вызвать функцию set_leds.

В реальности кроме pt_int32 родились только pt_mstring - malloc'ed strings, что тоже довольно удобно.

Вообще, надо сказать, темпы развития API классического Юникса меня несколько удивляют - есть ощущение, что никто им всерьёз не занимается, хотя, кажется, определённая систематизация ему явно не повредит.

У меня есть ещё несколько дополнений к традиционному POSIX-у, которые мне, Юниксоиду с 30-летним стажем, кажутся просто очевидными. Будет время - опубликую.

Ссылки на реализацию (кожа и кости, но всё же):

<ul>
	<li><a href="https://github.com/dzavalishin/phantomuserland/blob/master/include/kernel/properties.h">Структуры данных ядра</a></li>
	<li><a href="https://github.com/dzavalishin/phantomuserland/blob/master/phantom/apps/mode/mode.c">Команда mode</a></li>
	<li><a href="https://github.com/dzavalishin/phantomuserland/blob/c5ab56286ef098429a2bcef28da191118e0c6cbf/oldtree/kernel/phantom/keyboard.c#L425">Пример втыкания в простой драйвер</a></li>
	<li><a href="https://github.com/dzavalishin/phantomuserland/blob/master/oldtree/kernel/phantom/properties.c">Ядерная подсистема</a></li>
</ul>

Кажется очевидным, но упомяну, что механизм, в принципе, можно применять для любых сущностей, вовсе не только для устройств.
