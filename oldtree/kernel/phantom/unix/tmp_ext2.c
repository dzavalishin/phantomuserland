#if 0


/*!     @file fs/ext2/ext2.c
 *      @brief ext2 file system.
 *      @author
 *              Filippo Brogi
 *      @note Copyright (&copy;) 2003
 *              Filippo Brogi
 *      @date Last update:
 *              2003-09-30 by Andrea Righi
 *                      init_ext2() now returns a boolean value.\n
 */

#include <const.h>
#include <string.h>

#include <arch/i386.h>
#include <arch/mem.h>

#include <kernel/Ide.h>
#include <kernel/IdeDebug.h>
#include <kernel/IdeTimer.h> // must remove
#include <kernel/IdeLow.h>   // must remove
#include <kernel/keyboard.h>
#include <kernel/kmalloc.h>
#include <kernel/video.h>

#include <kernel/ext2.h>

#define min(a,b) (((a)<(b)) ? (a) : (b))


// contenuto di un blocco su disco si usa come variabile ausiliaria nelle func
byte *data_block;
// codice errore lettura da disco
int err;


bool mount_ext2 = FALSE; // TRUE se il montaggio del file system e' andato a buon fine
char path_ext2[1024];
int level;






//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//                  FUNZIONI AUSILIARIE

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<





/*
 lettura del descrittore di gruppo. In generale e' allocato
 consecutivamente al blocco del disco contenente il super
 blocco, cioe` blocco 1 della partizione
 */


// il primo descrittore di gruppo parte da 0
bool ReadGroupDescriptor(dword grp,struct group_descriptor* data){

    if (grp>number_of_groups)
    {
        return FALSE;
    }

    return TRUE;
}


//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//                  GESTIONE DESCRITTORI DI GRUPPO

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

// struttura tabella dei descrittori di gruppo






//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//                      GESTIONE DEGLI INODE

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@










//############################################################

//              GESTIONE DEI FILE

//############################################################

dword * ptr_dir;









//c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,

//              DIRECTORY MANAGEMENT

//c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,

// variabili per gestire gli elementi delle directory
word count_dir; // numero elementi della dir
char** ext2_file_name = NULL; // filename
dword* inode_dir = NULL; // inodes
byte* file_type_dir = NULL; //file type
char* text = NULL; // ext2_file_name[] punta dentro la stringa di caratteri
char* name = NULL;


// elementi che si usano per il backup della memoria
char path_ext2_backup[1024];
int level_backup;
dword ino_dir_backup,ino_current_dir;



// dato il nome trova l'inode (0 errore)
dword FindFile(char* cmp){
    int i;

    for (i=0;i<count_dir;i++)
        if (!strcmp(cmp,ext2_file_name[i]))
        {
            return inode_dir[i];
        }
    return 0;
}


// Opening a directory given its inode, FALSE = error
bool Open_Dir(struct i_node* inode)
{

    byte *ptr_block;
    dword block_dir; // blocchi che compongono la directory
    //dword *ptr_dir; // puntatori blocchi disco contenenti la directory
    byte *tmp; // memorizza il contenuto della direcotry
    int i,len,idx;
    struct dir_ff *dir;


    //controlliamo se si tratta effettivamente di una directory

    if (!(isDir(inode) || isFastSymbolicLink(inode)))
    {
        return FALSE;
    }

    // returns the memory allocated for the fields if you already used if (name = NULL)
    if (ext2_file_name != NULL)
    {
        free(inode_dir);
        free(file_type_dir);
        free(text);
        free(ext2_file_name);
    }

    // se tutto va bene si procede all'apertura memorizzando i puntatori
    // ai blocchi dati nell'area di memoria puntata da ptr_dir
    if (!(Open_File(inode,inode->i_mode & MODE_MASK)))
    {
        return FALSE;
    }


    // dimensione in blocchi della directory
    block_dir = (inode->i_size + dim_block - 1)/dim_block;

    // inserimento nella sezione di memoria tmp dei dati relativi alla directory
    tmp = (byte *)malloc(inode->i_size);
    memset(tmp,0,inode->i_size);


    //punta all'inizio dell'area di memoria tmp
    ptr_block = (byte *)tmp;

    //memset(ptr_block,0,dim_block);
    for (i = 0; i<block_dir;i++)
    {
        //SHOW_FLOW( 2, "ptr_dir[%u]: %u",i,ptr_dir[i]);
        if ( (err = ReadSectorsLba((int64_t)blkno_to_LBA(ptr_dir[i]),spb,(word *)ptr_block)) )
        {
            SHOW_FLOW( 2, "Error reading data block");
            ShowIdeErrorMessage(err,TRUE);
            free(ptr_dir);
            free(tmp);
            return FALSE;
        }
        ptr_block += dim_block;
    }

    free(ptr_dir);


    // pass 1: counting elements in the directory

    count_dir = 0; // memorizza il numero di elementi della dir
    len = 0;
    // I is increased by the size of the elements of the directory
    // Len length of the elements is incremented +1 for the end of string
    // Dir pointing at each step in the sequence of bytes of each element of the dir    for (i = 0; i<inode->i_size; )
    {
        dir = (struct dir_ff*)&tmp[i];
        if (dir->inode) // se uguale a zero vuol dire non utilizzato
        {
            //TODO: nel caso in cui non si stampino i fsl
            count_dir++;
            len += dir->name_len + 1;
        }
        i += dir->rec_len;
    }

    // vettore che contiene gli inode dei file nella directory
    inode_dir = (dword *)malloc(count_dir * sizeof(dword));
    memset(inode_dir,0,count_dir * sizeof(dword));
    // vettore che contiene il tipo dei file nella directory
    file_type_dir = (byte *)malloc(count_dir);
    memset(file_type_dir,0,count_dir);
    // nome dei file degli elementi della directory
    // *** Andrea Righi 2003-10-03 *********************************//
    // name = (char *)malloc(count_dir);
    // memset(name,0,count_dir);
    // ext2_file_name = &name;
    //**************************************************************//
    ext2_file_name = malloc(count_dir*sizeof(char *));

    text = (char *)malloc(len);
    memset(text,0,len);

    // secondo passo si registrano gli elementi
    idx = 0;
    for (len = i = 0;i<inode->i_size;)
    {
        dir = (struct dir_ff*)&tmp[i];
        // si memorizzano gli indirizzi degli inode degli elementi
        if (dir->inode)
        {
            inode_dir[idx] = dir->inode;
            file_type_dir[idx] = dir->file_type;
            // copia dell'elemento di memoria
            memcpy(&text[len],dir->name,dir->name_len);
            // si azzera l'elemento dopo il nome
            text[len+dir->name_len] = '\0';
            //punta al corrispondente nome di file
            ext2_file_name[idx++] = &text[len];
            len += dir->name_len + 1;
        }
        i += dir->rec_len;
    }

    // si restituisce la memoria occupata da tmp
    free(tmp);

    return TRUE;
}


/**************************************************************************
 *
 *           Procedure che definiscono comandi della shell per Ext2
 *
 ***************************************************************************/


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//                      Gestione date relative ai file

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

struct date_ext2{
    dword day;
    dword month;
    dword anno;
}data;

struct time_ext2{
    dword secondi;
    dword minuti;
    dword ora;
}tempo;


struct time_ext2 det_time(dword ino_time){
    struct time_ext2 t;
    dword s;

    s = ino_time % 86400;
    // ora
    t.ora = s / 3600;
    s -= t.ora*3600;
    // minuti
    t.minuti = (s / 60);
    s -= t.minuti*60;
    // secondi
    t.secondi = s;
    return(t);

}

struct date_ext2 det_date(dword ino_time){
    struct date_ext2 d;
    dword mg;

    ino_time = ino_time / 86400; //(60*60*24)

    // tempo in giorni trascorso dal 1/1/1970

    mg = ino_time + 1;

    // si determina l'anno
    for (d.anno=1970;;d.anno++)
    {
        if ((d.anno % 4)==0) // bisestile
        {
            if (mg<366) break;
            mg -= 366;
        }
        else
        {
            if (mg<365) break;
            mg -= 365;
        }

    }

    // mg contiene month e day
    for (d.month=1;;d.month++)
    {

        if ((d.month==4)||(d.month==6)||(d.month==9)||(d.month==11))
        {
            if (mg<=30) break;
            mg -= 30;
        }
        else if ((d.month==2)&&(d.anno%4==0))
        {
            if (mg<=29) break;
            mg -= 29;
        }
        else if ((d.month==2)&&(d.anno%4!=0))
        {
            if (mg<=28) break;
            mg -= 28;
        }
        else
        {
            if (mg<=31) break;
            mg -= 31;
        }
    }

    d.day = mg;
    return(d);

}




// restituisce la path corrente a partire dalla radice
char *pwd_ext2(){
    if (mount_ext2)
        return(path_ext2);
    else return('\0');
}


// restituisce l'elenco dei file della directory corrente
void ls_ext2(){
    int i;
    struct i_node* info_file;
    if (mount_ext2){

        clrscr();
        //SHOW_FLOW( 2, " Total: %u",count_dir);
        for (i=0; i<count_dir; i++)
        {

            info_file = get_inode(inode_dir[i]);
            // data e ora di creazione del file
            tempo = det_time(info_file->i_ctime);
            data = det_date(info_file->i_ctime);

            // TODO : fast symbolic link
            if ((info_file->i_mode&MODE_MASK)==MODE_LINK)
                continue;
            switch (info_file->i_mode & MODE_MASK)
            {

            case MODE_FILE:
                set_color(WHITE);
                break;

            case MODE_DIR:
                set_color(LIGHT_BLUE);
                break;

            case MODE_CDEV:
                set_color(LIGHT_GREEN);
                break;

            case MODE_BDEV:
                set_color(LIGHT_RED);
                break;

            case MODE_FIFO:
                set_color(LIGHT_MAGENTA);
                break;

            case MODE_SOCK:
                set_color(LIGHT_CYAN);
                break;

            case MODE_LINK:
                set_color(RED);
                break;

            }

            SHOW_FLOW( 2, " %s",ext2_file_name[i]);
            set_color(DEFAULT_COLOR);
            kprintf ("\r                    (%u)",inode_dir[i]);
            set_color(WHITE);
            kprintf("\r                                     %u",info_file->i_size);
            kprintf("\r                                             %u:%u",tempo.ora,tempo.minuti);
            kprintf("\r                                                      %u-%u-%u",data.anno,data.month,data.day);
            kprintf("\r                                                                     %u",info_file->i_blocks);
            set_color(DEFAULT_COLOR);
            if (((i+1)%22)==0)
            {
                SHOW_FLOW( 2, " ------ Continue ------");
                if (kgetchar()==CTRL_C)
                {
                    kprintf("\n\n\r");
                    return;
                }
                clrscr();
            }
        }
        kprintf("\n\n");
    }
    else SHOW_FLOW( 2, "Unmounted File System Ext2\n\n\r");
    return;
}



// permette di cambiare la directory corrente
void cd_ext2(char *param){
    struct i_node* ino;
    int i,j_param,len_param,i_param;
    dword ino_dir;
    char elem_path[256]; // si usa per il parsing della path
    bool errore;

    //SHOW_FLOW( 2, "param %s",param);
    if(!mount_ext2)
    {
        SHOW_FLOW( 2, "Unmounted File System Ext2\n\n\r");
        return;
    }

    if (!strcmp(param,"."))
    {
        // si rimane nella directory corrente
        return;
    }

    if (!strcmp(param,"/"))
    {
        // si ritorna alla directory radice
        if (!Open_Dir(get_inode(EXT2_ROOT_INO)))
        {
            // se non si puo` piu` aprire la root si smonta il file system
            mount_ext2 = FALSE;
            return;
        }
        level = 0;
        path_ext2[0]='\0';
        return;
    }


    if ((level == 0)&&!strcmp(param,".."))
    {
        SHOW_FLOW( 2, "Cannot go up from root\n\n\r");
    }
    else if ((level ==1)&&!strcmp(param,".."))
    {
        // si ritorna alla directory radice
        if (!Open_Dir(get_inode(EXT2_ROOT_INO)))
        {
            // se non si puo` piu` aprire la root si smonta il file system
            mount_ext2 = FALSE;
            return;
        }
        level = 0;
        path_ext2[0]='\0';
        ino_current_dir = EXT2_ROOT_INO;
    }
    else
    {

        errore = FALSE;
        // dobbiamo effettuare il salvataggio dei componenti
        memcpy(&path_ext2_backup,&path_ext2,1024);
        //SHOW_FLOW( 2, "path_ext2_backup : %s\n\r",path_ext2_backup);
        level_backup = level;
        ino_dir_backup = ino_current_dir;

        // dobbiamo fare il parsing della path
        i_param = 0;
        if (param[0]=='.'&&param[1]=='/')
        {
            // ./ sta ad indicare restiamo nella dir corrente
            // non ha senso riaprirla
            i_param = 2;
        }
        if (param[strlen(param)-1]=='/')
        {
            len_param = strlen(param)-1;
        }
        else len_param = strlen(param);
        //SHOW_FLOW( 2, "len_param : %u\n\r",len_param);

        for (j_param=0;i_param<=len_param;i_param++)
            if ((param[i_param]=='/')||i_param==len_param)
            {
                elem_path[j_param]='\0';
                j_param = 0;
                //SHOW_FLOW( 2, "elem_path : %s\n\r",elem_path);
                if (!(ino_dir=FindFile(elem_path)))
                {
                    // non esiste nella directory corrente la sottodirectory param
                    SHOW_FLOW( 2, "Directory no match\n\n\r");
                    errore = TRUE;
                    break;
                }
                else if (!(ino=get_inode(ino_dir)))
                {
                    // invalid inode number
                    errore = TRUE;
                    break;
                }
                else if (!(isDir(ino)||isFastSymbolicLink(ino)))
                {
                    SHOW_FLOW( 2, "Not a directory\n\n\r");
                    errore = TRUE;
                    break;
                }
                else if (!Open_Dir(ino))
                {
                    if (!isFastSymbolicLink(ino))
                        SHOW_FLOW( 2, "Open failed\n\n\r");
                    errore = TRUE;
                    break;
                }
                else
                {
                    // si aggiorna la path salendo nell'albero
                    if (!strcmp(elem_path,".."))
                    {

                        if (level)
                        {
                            level--;
                            for (i=strlen(path_ext2)-2;path_ext2[i]!='/';i--);
                            path_ext2[i+1]='\0';
                        }
                        if (!level)
                            path_ext2[0]='\0';
                    }
                    else
                    {
                        strcat(path_ext2,elem_path);
                        strcat(path_ext2,"/");
                        level++;
                    }
                    ino_current_dir = ino_dir;

                }
            }
            else elem_path[j_param++] = param[i_param];

        if (errore)
        {
            // si ripristina la vecchia directory
            SHOW_FLOW( 2, "errore\n\r");
            level = level_backup;
            memcpy(&path_ext2,&path_ext2_backup,1024);
            ino_current_dir = ino_dir_backup;
            Open_Dir(get_inode(ino_current_dir));


        }
        SHOW_FLOW( 2, " level :%u\n\r",level);

    }
}

void cat_ext2(char *param)
{
    struct i_node * ino;
    dword i;
    word block_file;
    char *c=NULL;
    int stop=0,ll=0;
    dword ino_file;

    if (!mount_ext2)
    {
        SHOW_FLOW( 2, "Unmounted File System Ext2\n\r");
        return;
    }

    if (!(ino_file = FindFile(param)))
    {
        SHOW_FLOW( 2, "No such file\n\r");
        return;
    }
    else if (!(ino=get_inode(ino_file)))
    {
        // invalid inode number
    }
    else if (!(isFile(ino)||isFastSymbolicLink(ino)))
    {
        SHOW_FLOW( 2, "Not a regular file\n\r");
        return;
    }
    else if (Open_File(ino,ino->i_mode & MODE_MASK))
    {
        // dimensione in blocchi del file
        block_file = 0;
        stop = 3;
        data_block = (byte *)malloc(dim_block);
        memset(data_block,0,dim_block);
        //clrscr();
        set_color(LIGHT_BLUE);
        kprintf("filename: %s\n\n\r",param);
        set_color(DEFAULT_COLOR);
        for (i=0;i<ino->i_size;i++)
        {
            if (i % dim_block ==0)
            {
                if ( (err = ReadSectorsLba((int64_t)blkno_to_LBA(ptr_dir[block_file]),spb,(word *)data_block)) )
                {
                    SHOW_FLOW( 2, "Error reading file data block");
                    ShowIdeErrorMessage(err,TRUE);
                    free(ptr_dir);
                    free(data_block);
                    return;
                }
                block_file++;
                c = (char *)data_block;
            }


            if (*c==10) // carattere di fine linea
            {
                stop++;
                SHOW_FLOW( 2, "");
                ll=0;
            }
            else
            {
                kputchar(*c);
                if(ll++>70)
                {
                    // se la linea eccede in lunghezza la dimensione
                    //dello schermo si divide in due linee
                    stop++;
                    ll = 0;
                }
            }

            c++;

            if (((stop)%22)==0)
            {
                kprintf("\n\r ------ Continue ------");
                if ( kgetchar()==CTRL_C )
                {
                    // *** Andrea Righi 2003-10-04 *** //
                    free(data_block);
                    //********************************//
                    SHOW_FLOW( 2, "");
                    return;
                }
                clrscr();
                stop = 3;
                set_color(LIGHT_BLUE);
                kprintf("filename: %s\n\n\r",param);
                set_color(DEFAULT_COLOR);
            }
        }
        free(data_block);
    }
}



/**************************************************************************
 *
 *           Procedure di inizializzazione del file system Ext2
 *
 ***************************************************************************/






#endif


