#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX_PATH_LEN 256
#define MAX_TASK_NAME 251
#define MAX_NTASKS 1000
#define MAX_TASK_DISPLAY 20

const char FNAME[] = "todolist"; //save file name (contains \0)
//this is probably what you think it is; formatting %d is for MAX_TASK_DISPLAY
const char HELP_TEXT[] = "TODO: A simple to-do list program \n\n\
Usage: todo [command] [arguments]\n\
Options\n\
  <none>\t\t\tShow at most the first %d elements of the to-do list\n\
  --all\t\t\t\tShow all the elements of the to-do list\n\
  -h, --help\t\t\tShow this menu\n\
  -a, --add [priority] [task]\tAdd task to the to-do list\n\
  -r, --remove [id]\t\tRemove task from the to-do list\n\
  -p, --priority [id]\t\tModify priority of a task\n\
  --getfpath\t\t\tPrint save file path\n";

typedef struct {
    uint16_t id;
    int16_t priority;
    char name[MAX_TASK_NAME];
} Task;

uint16_t kyu_len;
Task kyu[MAX_NTASKS]; //Priority queue (aka max heap) structure

//The base of all heaps of things we can do with heaps.
//Only call with i < kyu_len, otherwise bad things can happen
void heapify(uint16_t i) {
    while(1) {
        //for idx i, left is 2i+1, right is 2i+2
        uint16_t best_i = i;
        int16_t best_pri = kyu[i].priority;
        if (2*i+2 < kyu_len && kyu[2*i+2].priority > best_pri) {
            best_i = 2*i+2;
            best_pri = kyu[2*i+2].priority;
        }
        if (2*i+1 < kyu_len && kyu[2*i+1].priority > best_pri) {
            best_i = 2*i+1;
            //best_pri = kyu[2*i+1].priority is not necessary: best_pri not used later
        }

        //we found a higher priority child: keep heapifying
        if (best_i > i) {
            //swap with the highest priority child
            Task tmp = kyu[i];
            kyu[i] = kyu[best_i];
            kyu[best_i] = tmp;

            i = best_i;
        }
        else break; //done heapifying
    }
}

//Increase priority of an element to the heap, given by its index
void increase_priority(uint16_t i, int16_t new_priority)
{
    if (kyu_len <= i) //element doesn't exist (shouldn't happen)
    {
        printf("ERROR in increase_priority: element does not exist!\n");
        exit(1);
    }
    if (new_priority < kyu[i].priority) //trying to decrease priority (also shouldn't happen)
    {
        printf("ERROR: Trying to decrease priority with increase_priority\n");
        exit(1);
    }

    kyu[i].priority = new_priority;

    //restore heap structure
    //parent node of i: floor(i-1)/2
    while(i > 0 && kyu[(i-1)/2].priority < kyu[i].priority)
    {
        //swap node with its parent
        Task tmp = kyu[i];
        kyu[i] = kyu[(i-1)/2];
        i = (i-1)/2; //go to parent node
        kyu[i] = tmp;
    }
}

//Add a task into the heap
void add(Task t)
{
    //there is no more room! (shouldn't happen)
    if (kyu_len == MAX_NTASKS)
    {
        printf("ERROR: too many tasks!!\n");
        exit(1);
    }
    
    //add element at the end with lowest priority
    kyu[kyu_len] = t;
    kyu[kyu_len++].priority = INT16_MIN; //lowest priority
    increase_priority(kyu_len - 1, t.priority); //do it with the right priority
}

//Save the todo list PROPERLY
//NOTE: why does this make an error when doing -a?
void save_kyu(char* fpath)
{
    FILE* fptr = fopen(fpath, "wb");
    if(!fptr)
    {
        printf("ERROR: could not save to-do list to file\n");
        exit(1);
    }
    fwrite(kyu, 1, kyu_len*sizeof(Task), fptr);
    fclose(fptr);
}

int main(int argc, char** argv)
{
    //todo -h or todo --help
    if (argc == 2 && (
        (strlen(argv[1]) == 2 && !strncmp(argv[1], "-h", 2))
        || (strlen(argv[1]) == 6 && !strncmp(argv[1], "--help", 6))
        ))
    {
        printf(HELP_TEXT, MAX_TASK_DISPLAY);
        return 0;
    }

    //get current directory, where program is located
    char fpath[MAX_PATH_LEN + sizeof(FNAME) + 1];
    const size_t cur_dir_cap = sizeof(fpath) - sizeof(FNAME) + 1; //enough space to insert FNAME; +1 is to account for \0 only getting moved
    //POTENTIAL VULNERABILITY: readlink
    int cur_dir_size = MIN(readlink("/proc/self/exe", fpath, cur_dir_cap), cur_dir_cap - 1);
    if (cur_dir_size == cur_dir_cap - 1) { //1 character of margin, to make sure no truncation is going on
        printf("ERROR: executable path cannot be longer than %lu characters\n", cur_dir_cap - 2);
        return 1;
    }
    else if(cur_dir_size > 0)
        fpath[cur_dir_size] = '\0';
    else if(cur_dir_size == 0) { //why would anyone ever want to do this?
        fpath[0] = '/';
        cur_dir_size = 1;
    }
    else { //readlink returned -1: error
        printf("ERROR when trying to get executable path\n");
        return 1;
    }

    //go to last slash, and insert "todolist" (save file name)
    int last_slash = cur_dir_size - (cur_dir_size > 0); //don't access array at index -1
    while (last_slash > 0 && fpath[last_slash] != '/') last_slash--;
    for (int i = 0; i < 9; i++) fpath[last_slash + 1 + i] = FNAME[i];

    //todo --getfpath
    if (argc == 2 && strlen(argv[1]) == 10 && !strncmp(argv[1], "--getfpath", 10))
    {
        printf("%s\n", fpath);
        return 0;
    }

    //create todolist file if does not exist
    //POTENTIAL VULNERABILITY: fopen with CONTROLLABLE input!
    //I'm not gonna be repeating that...
    FILE* fptr = fopen(fpath, "rb"); //check if file exists
    if(!fptr) { //no: create it
        fptr = fopen(fpath, "wb");
        if(!fptr) {
            printf("ERROR: could not access file\n");
            return 1;
        } else { //ok: reopen in read mode
            fclose(fptr); //close write mode file
            fptr = fopen(fpath, "rb");
        }
    }

    //verify the file has properly been opened
    if (!fptr)
    {
        printf("ERROR: could not open file\n");
        return 1;
    }

    //get file size
    fseek(fptr, 0, SEEK_END);
    size_t fsize = ftell(fptr);
    //file validity check: size
    if (fsize % sizeof(Task) || fsize > sizeof(kyu)) {
        printf("ERROR: invalid file\n");
        fclose(fptr);
        return 1;
    }
    rewind(fptr);

    char* contents = (char*)malloc(fsize * sizeof(char));
    fread(contents, 1, fsize, fptr);

    //load up the actual priority queue
    //kyu_len = fsize / sizeof(Task); //number of tasks (perfect division and <= MAX_NTASKS)
    //memset(kyu, 0, sizeof(kyu)); //clear the entire mem space (unnecessary probably)
    //memcpy(kyu, contents, fsize); //NOTE: this has previously been bounds-checked!

    //do it while heapifying
    Task* data = (Task*)contents;
    kyu_len = 0;
    for (uint16_t i = 0; i < fsize / sizeof(Task); i++)
        add(data[i]); //don't worry, bounds-checking has already been done

    //cleanup crew!
    fclose(fptr);
    free(contents);

    //No arguments: show the entire to-do list
    if (argc == 1)
    {
        if (kyu_len == 0)
        {
            printf("No tasks\n");
            return 0;
        }
        printf("%d tasks\n", kyu_len);
        printf("    ID Priority Name\n");

        //sort elements by priority
        for (int i = MIN(kyu_len, MAX_TASK_DISPLAY); i > 0; i--)
        {
            //print highest priority task
            Task task = kyu[0];
            printf("%6u%9d %s\n", task.id, task.priority, task.name);

            //pop that task            
            kyu[0] = kyu[--kyu_len];
            heapify(0);
        }
        if (kyu_len) printf("... (%d more)\n", kyu_len);
    //todo --all: display ALL the tasks!
    } else if (argc == 2 && strlen(argv[1]) == 5 && !strncmp(argv[1], "--all", 5)){
        if (kyu_len == 0)
        {
            printf("No tasks\n");
            return 0;
        }
        printf("%d tasks\n", kyu_len);
        printf("    ID Priority Name\n");

        //sort elements by priority
        for (int i = kyu_len; i > 0; i--)
        {
            //print highest priority task
            Task task = kyu[0];
            printf("%6u%9d %s\n", task.id, task.priority, task.name);

            //pop that task            
            kyu[0] = kyu[--kyu_len];
            heapify(0);
        }
    } else if (argc >= 3 && ( //todo -a, --add: add a task
        (strlen(argv[1]) == 2 && !strncmp(argv[1], "-a", 2))
        || (strlen(argv[1]) == 5 && !strncmp(argv[1], "--add", 5))
    )) {
        //check if there is space
        if (kyu_len == MAX_NTASKS)
        {
            printf("You can have at most %d tasks\n", MAX_NTASKS);
            return 1;
        }

        //check if argv[2] is an integer
        if (strlen(argv[2]) > 6) { //too big or too small for sure!
            printf("ERROR: priority must be an integer between -10000 and 10000\n");
            return 1;
        }
        uint16_t prio_len = strlen(argv[2]);
        for (uint16_t i = 0; i < prio_len; i++)
        {
            char c = argv[2][i];
            if ((i != 0 || c != '-') && (c < '0' || c > '9'))
            {
                printf("ERROR: priority must be an integer between -10000 and 10000\n");
                return 1;
            }
        }
        int32_t prio_big = atoi(argv[2]); //we need extra capacity for now, so int32_t (number could be 69420)
        if (prio_big < -10000 || prio_big > 10000) //too big or too small!
        {
            printf("ERROR: priority must be an integer between -10000 and 10000\n");
            return 1;
        }

        Task t;
        t.priority = prio_big;

        //parse the task's name (destroys argv[3:])
        int cur_arg = 3; //first argument in the name
        memset(t.name, 0, MAX_TASK_NAME); //last 0 to verify if name short enough
        for (int i = 0; i < MAX_TASK_NAME; i++)
        {
            t.name[i] = *(argv[cur_arg]++);
            if (!t.name[i]) { //null character: switch to next arg
                //last argument?
                if (++cur_arg < argc) t.name[i] = ' '; //just insert a space
                else break; //end of arguments reached!
            }
        }
        if (t.name[MAX_TASK_NAME - 1] != 0) //too long!
        {
            printf("ERROR: Task name can be at most %d characters\n", MAX_TASK_NAME - 1);
            return 1;
        }

        //find t.id: smallest possible id not in kyu already
        //can it be done in O(1) additional space without destroying the original array?
        //idk, all i do know is that its possible to do it with bools
        //NOTE: my implementation is only 12.5% efficient, but it works well enough
        uint8_t in_there[MAX_NTASKS];
        memset(in_there, 0, sizeof(in_there)); //zero out array
        for (int i = 0; i < kyu_len; i++) //loop over array and set taken ids to 42
            if(kyu[i].id < MAX_NTASKS)
                in_there[kyu[i].id] = 42;
        for(t.id = 0; in_there[t.id]; t.id++); //look for smallest 0 (otherwise MAX_NTASKS - 1)
        //NOTE: above loop never goes wrong, as there are at most MAX_NTASKS - 1 nonzero elements in there

        add(t); //got id, name, priority, we gud2go
        save_kyu(fpath); //and... SAVE!
    } else if (argc == 3 && ( //todo -r, --remove: remove a task
        (strlen(argv[1]) == 2 && !strncmp(argv[1], "-r", 2))
        || (strlen(argv[1]) == 8 && !strncmp(argv[1], "--remove", 8))
    )) {
        //check if argv[2] is a positive integer
        if (strlen(argv[2]) > 5) { //too big for sure!
            printf("ERROR: invalid id\n");
            return 1;
        }
        uint16_t id_len = strlen(argv[2]);
        for (uint16_t i = 0; i < id_len; i++)
        {
            char c = argv[2][i];
            if (c < '0' || c > '9')
            {
                printf("ERROR: invalid id\n");
                return 1;
            }
        }
        int32_t id = atoi(argv[2]); //we need extra capacity for now, so int32_t (number could be 69420)
        if (id < 0 || id > MAX_NTASKS) //too big or too small (impossible)!
        {
            printf("ERROR: invalid id\n");
            return 1;
        }

        //find index
        uint16_t index = UINT16_MAX;
        for(int i = 0; i < kyu_len; i++)
            if(kyu[i].id == id)
            {
                index = i;
                break;
            }

        if (index == UINT16_MAX) //not found: ERROR!
        {
            printf("ERROR: invalid id\n");
            return 1;
        }

        //increase to-delete element priority all the way, then pop it
        increase_priority(index, INT16_MAX);
        kyu[0] = kyu[--kyu_len];
        heapify(0);
        save_kyu(fpath); //and... SAVE!
    } else if (argc == 4 && ( //todo -p, --priority: change task priority
        (strlen(argv[1]) == 2 && !strncmp(argv[1], "-p", 2))
        || (strlen(argv[1]) == 10 && !strncmp(argv[1], "--priority", 10))
    )) {
        //check if argv[2] is a positive integer
        if (strlen(argv[2]) > 5) { //too big for sure!
            printf("ERROR: invalid id\n");
            return 1;
        }
        uint16_t id_len = strlen(argv[2]);
        for (uint16_t i = 0; i < id_len; i++)
        {
            char c = argv[2][i];
            if (c < '0' || c > '9')
            {
                printf("ERROR: invalid id\n");
                return 1;
            }
        }
        int32_t id = atoi(argv[2]); //we need extra capacity for now, so int32_t (number could be 69420)
        if (id < 0 || id > MAX_NTASKS) //too big or too small (impossible)!
        {
            printf("ERROR: invalid id\n");
            return 1;
        }

        //find index
        uint16_t index = UINT16_MAX;
        for(int i = 0; i < kyu_len; i++)
            if(kyu[i].id == id)
            {
                index = i;
                break;
            }

        if (index == UINT16_MAX) //not found: ERROR!
        {
            printf("ERROR: invalid id\n");
            return 1;
        }

        //do the same stuff with priority
        //check if argv[3] is an integer
        if (strlen(argv[3]) > 6) { //too big or too small for sure!
            printf("ERROR: priority must be an integer between -10000 and 10000\n");
            return 1;
        }
        uint16_t prio_len = strlen(argv[3]);
        for (uint16_t i = 0; i < prio_len; i++)
        {
            char c = argv[3][i];
            if ((i != 0 || c != '-') && (c < '0' || c > '9'))
            {
                printf("ERROR: priority must be an integer between -10000 and 10000\n");
                return 1;
            }
        }
        int32_t prio = atoi(argv[3]); //we need extra capacity for now, so int32_t (number could be 69420)
        if (prio < -10000 || prio > 10000) //too big or too small!
        {
            printf("ERROR: priority must be an integer between -10000 and 10000\n");
            return 1;
        }

        Task t = kyu[index]; //new task to be added back
        t.priority = prio; //same but with different priority

        //universal priority modification: delete element and add it back with different priority
        //increase to-delete element priority all the way, then pop it
        increase_priority(index, INT16_MAX);
        kyu[0] = kyu[--kyu_len];
        heapify(0);
        add(t); //add back the task
        save_kyu(fpath); //and... SAVE!
    } else {
        printf("Unknown command. Use todo -h to get a list of available commands.\n");
    }
}
