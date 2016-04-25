#include "commit.h"
#include "stage.h"
#include "delta.h"
#include "path.h"
#include "project-init.h"
#include "global.h"
#include "show-tables.h"
#include "help.h"

#include <math.h>

enum cmd_type {
    CREATE,
    INSERT,
    COMMIT,
    REVERT,
    RST,
    HIST,
    HEAD,
    STATUS,
    SHOW,
    COMPARE,
    CHECKOUT,
    SEE,
    TAG,
    SET,
    LIST,
    HELP,
    INVALID,
    USER,
    UNKNOWN
};

#define CREATE_DESC "create a peg repository"
#define INSERT_DESC "insert the files into stage area or temporary database"
#define COMMIT_DESC "commit the changes"
#define RESET_DESC  "remove the files present in the stage area"
#define REVERT_DESC "revert the local changes"
#define SHOW_DESC   "displays the tables"
#define COMPARE_DESC "shows the changes made till the last commit"
#define STATUS_DESC "shows the status of the project directory"
#define HIST_DESC "logs the commits"
#define LIST_DESC "displays the files in the last commit"
#define TAG_DESC "apply a tag to a commit"
#define HELP_DESC "shows the help for a command"
#define HEAD_DESC "change the position of the head pointer"
#define CHECKOUT_DESC "apply the version change command"

struct command_type {
    enum cmd_type t;
    char *cmd;
    char *desc;
};

static struct command_type cmds[] = {
    { CREATE, "create", CREATE_DESC },
    { INSERT, "insert", INSERT_DESC },
    { COMMIT, "commit", COMMIT_DESC },
    { RST, "reset", RESET_DESC },
    { HIST, "hist", HIST_DESC },
    { HEAD, "head", HEAD_DESC },
    { STATUS, "status", STATUS_DESC },
    { SHOW, "show", SHOW_DESC },
    { REVERT, "revert", REVERT_DESC },
    { COMPARE, "compare", COMPARE_DESC },
    { CHECKOUT, "checkout", CHECKOUT_DESC },
    { LIST, "list", LIST_DESC },
    { TAG, "tag", TAG_DESC },
    { HELP, "help", HELP_DESC },
    { SEE, "seeChanges" },
    { SET, "setAlias" }
};

#define COMMAND_COUNT 16

static int argc;
static char **argv;

struct command {
    struct strbuf name;
    enum cmd_type type;
    int match;
    int argc;
    char **argv;
};

struct command_alias {
    struct strbuf alias;
    struct command *cmd;
};

struct core_commands {
    int count;
    int toexecute;
    struct command cmd;
    struct core_commands *next;
};

/**
 * various commands look like sql commands
 *  1) Command to initialize project
 *        peg create repo [here | path/to/project]
 *  2) Command to reinitialize project
 *        peg create repo [here | path/to/project]
 *  3) Command to add a file
 *        peg insert [files | directories | all]
 *         [files => file | files]
 *         [directories => directory | directories]
 *      Note: only changed files are added to stage area
 *  4) Command to commit a change
 *        peg commit message="some message" (description="some description" |
 *                            tag="v1.x.x.x")
 *  5) Command to revert a file
 *        peg revert <filename> to nth commit
 *  6) Command to see the changes you have made
 *        peg see changes in <filename>
 *  7) Command to compare two commits
 *        peg compare <commit1> <commit2>
 *  8) Command to compare a particular commit with HEAD commit
 *        peg compare <commit> with latest
 *  9) Command to set aliases
 */

extern void reset_head_command(int argc, char *argv[]);

void init_command(struct command *cmd)
{
    strbuf_init(&cmd->name, 0);
    cmd->type = UNKNOWN;
    cmd->argc = 0;
    cmd->argv = NULL;
}


void warn_no_user_config()
{
    fprintf(stderr, YELLOW"warning"RESET": no user name provided.\n");
}

void create_peg_environment()
{
    struct config_list *node;

    create_environment(&environment);
    read_config_file(&environment);
    parse_config_file(&environment);
    node = get_environment_value(&environment, "username");

    if (!node) {
        warn_no_user_config();
        exit(-1);
    }
    environment.owner = node->value.buf;
    node = get_environment_value(&environment, "useremail");

    if (!node) {
        die("no email provided in %s\n", environment.peg_config_filepath);
    }
    environment.owner_email = node->value.buf;
    node = get_environment_value(&environment, "password");

    if (node)
        environment.pswd = true;
}

void set_environment_value(const char *key, const char *value)
{

}

enum cmd_type find_command(struct core_commands *cmds, const char *cmd)
{
    struct core_commands *node = cmds;

    while (node) {
        if (!strcmp(node->cmd.name.buf, cmd))
            return node->cmd.type;
        node = node->next;
    }
    return UNKNOWN;
}

void read_user_cmds(struct core_commands **head, struct core_commands *last)
{

}

void gen_core_commands(struct core_commands **head)
{
    struct core_commands *last;
    struct core_commands *new;

    *head = NULL;
    for (int i = 0; i < COMMAND_COUNT; i++) {
        new = malloc(sizeof(struct core_commands));
        init_command(&new->cmd);
        strbuf_addstr(&new->cmd.name, cmds[i].cmd);
        new->cmd.type = cmds[i].t;
        new->toexecute = false;
        new->count = 0;
        new->next = NULL;
        if (!*head) {
            *head = new;
        } else {
            last->next = new;
        }
        last = new;
        (*head)->count++;
    }

    read_user_cmds(head, last);
}

int skip_command_prefix(struct strbuf *args)
{
    int i = 0;
    while (i < args->len && args->buf[i++] != ' ')
        ;
    return i;
}

enum match_type {
    MATCHED,
    UNMATCH,
    END
};

void suggest_create_usage()
{
    printf(" May be you wanted to say 'peg create repo ...'\n");
    printf("    (See 'peg help create' for proper syntax)\n");
}

bool help(int argc, char **argv)
{
    if (argc == 1) {
        usage("peg help <command>\n"
        " (`peg --help` displays all the peg commands)\n");
        return true;
    }
    display_help(argv[1]);
    return true;
}

bool parse_set_cmd(int argc, char **argv)
{
    return true;
}

bool create_tag(int argc, char *argv[])
{
    if (argc == 1) {
        list_tags();
        return true;
    }

    if (argc == 2)
        set_tag(NULL, 0, argv[1]);
    else set_tag(argv[1], strlen(argv[1]), argv[2]);
    return true;
}

bool exec_commands_args(enum cmd_type cmd, int count, char **in)
{
    size_t peek;
    switch (cmd) {
        case CREATE:
            return initialize_empty_project(count, in);
        case INSERT:
            return stage_main(count, in);
        case REVERT:
            return checkout(count, in);
        case COMMIT:
            return commit(count, in);
        case HELP:
            return help(count, in);
        case COMPARE:
            return delta_main(count, in);
        case SEE:
            return delta_main(count, in);
        case SET:
            return parse_set_cmd(count, in);

        case RST:
            {
                struct cache_object co;
                cache_object_init(&co);
                cache_object_clean(&co);
                exit(0);
            }

        case STATUS:
            return status_main(count, in);
        case TAG:
            return create_tag(count, in);
        case HIST:
        {
            print_commits();
            break;
        }
        case LIST:
            return list_index(count, in);

        case SHOW:
            return show_tables(count, in);

        case HEAD:
            reset_head_command(count, in);
            return 0;

        case CHECKOUT:
            status(".");
            revert_files_hard();
            return 0;

        case USER:
        case INVALID:
        case UNKNOWN:
        {}
    }
    return 0;
}

void exec_cmd(enum cmd_type cmd, int argc, char **argv)
{
    exec_commands_args(cmd, argc, argv);
}

void print_help()
{
    printf("Available `peg` commands: \n");
    for (int i = 0; i < COMMAND_COUNT - 2; i++) {
        printf("  "YELLOW"%10s"RESET"\t%s\n", cmds[i].cmd, cmds[i].desc);
    }
    printf("( use <PEG HELP ALL> to get a list of all \ncommands with their syntax and functionality)\n ");
}

int main(int argc, char *argv[])
{
    struct core_commands *head;
    enum cmd_type t;

    if (argc == 1 || !strcmp(argv[1], "--help")) {
        print_help();
        exit(0);
    }
    if (strcmp(argv[1], "create")) {
        struct stat st;
        if (stat(".peg", &st)) {
            die("Not a peg repository.\n");
        }
    }
    if (argc < 2) {
        die("No arguments provided.\n");
    }

    create_peg_environment();
    gen_core_commands(&head);
    t = find_command(head, argv[1]);
    if (t == UNKNOWN) {
        fprintf(stderr, "peg: '"RED"%s"RESET"' unknown command.\n", argv[1]);
        // suggest_commands(argv[1]);
        exit(0);
    }
    exec_cmd(t, --argc, (argv + 1));
    return 0;
}
