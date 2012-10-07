/*
 * esh - the 'pluggable' shell.
 *
 * Developed by Godmar Back for CS 3214 Fall 2009
 * Virginia Tech.
 */
#include <stdio.h>
#include <readline/readline.h>
#include <unistd.h>
#include <sys/wait.h>
#include "esh-sys-utils.h"
#include "esh.h"

static void
usage(char *progname)
{
    printf("Usage: %s -h\n"
        " -h            print this help\n"
        " -p  plugindir directory from which to load plug-ins\n",
        progname);

    exit(EXIT_SUCCESS);
}

/* Build a prompt by assembling fragments from loaded plugins that 
 * implement 'make_prompt.'
 *
 * This function demonstrates how to iterate over all loaded plugins.
 */
static char *
build_prompt_from_plugins(void)
{
    char *prompt = NULL;
    struct list_elem * e = list_begin(&esh_plugin_list);

    for (; e != list_end(&esh_plugin_list); e = list_next(e)) {
        struct esh_plugin *plugin = list_entry(e, struct esh_plugin, elem);

        if (plugin->make_prompt == NULL)
            continue;

        /* append prompt fragment created by plug-in */
        char * p = plugin->make_prompt();
        if (prompt == NULL) {
            prompt = p;
        } else {
            prompt = realloc(prompt, strlen(prompt) + strlen(p) + 1);
            strcat(prompt, p);
            free(p);
        }
    }

    /* default prompt */
    if (prompt == NULL)
        prompt = strdup("esh> ");

    return prompt;
}

/*
 * Returns the list of current jobs
 */
static struct list * get_jobs(void)
{
    return &current_jobs;
}

/*
v * Searches the pipeline and returns the job corresponding to jid
 * Returns the job, or NULL if not found
 */
static struct esh_pipeline * get_job_from_jid(int jid)
{
    struct list_elem *e;
    for (e = list_begin(&current_jobs); e != list_end(&current_jobs); e = list_next(e)) {
	struct esh_pipeline *job = list_entry(e, struct esh_pipeline, elem);
	if (job->jid == jid)
	    return job;	
    }

    return NULL;
}

/*
 * Searches the pipeline and return the job corresponding to pgrp
 * Returns the job, or NULL if not found
 */
static struct esh_pipeline * get_job_from_pgrp(pid_t pgrp)
{
    struct list_elem *e;
    for (e = list_begin(&current_jobs); e != list_end(&current_jobs); e = list_next(e)) {
	struct esh_pipeline *job = list_entry(e, struct esh_pipeline, elem);
	if (job->pgrp == pgrp)
	    return job;	
    }

    return NULL;
}

/* static void signal_handler(int signo, siginfo_t *info, void *ctxt) */
/* { */

/*     // sigint 2 sigtstp 20 */
/*     //if (signo == 2) */
	
/*     //    printf("Signal Handler Called"); */
/*     printf("\n"); */
/* } */
    

/* /\* */
/*  * Searches the commands (processes) and returns the process */
/*  * corresponding to pid */
/*  *\/ */
/* static struct esh_command * get_cmd_from_pid(pid_t pid) */
/* { */
/*     // */
/* } */

/*
 * Prints out the commands for a job
 */
static void print_job_commands(struct list jobs)
{
    struct list_elem *e = list_begin(&jobs);
    struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);
    
    for (e = list_begin(&pipeline->commands); e != list_end(&pipeline->commands); e = list_next(e)) {

	struct esh_command *command = list_entry(e, struct esh_command, elem);

	char **argv = command->argv;
	while (*argv) {
	    printf("%s ", *argv);
	    argv++;
	}

	if (list_size(&pipeline->commands) > 1)
	    printf("| ");
    }
}

/**
 * Assign ownership of ther terminal to process group
 * pgrp, restoring its terminal state if provided.
 *
 * Before printing a new prompt, the shell should
 * invoke this function with its own process group
 * id (obtained on startup via getpgrp()) and a
 * sane terminal state (obtained on startup via
 * esh_sys_tty_init()).
 *
 * Taken from Dr. Back's Snippet.
 */
static void
give_terminal_to(pid_t pgrp, struct termios *pg_tty_state)
{
    esh_signal_block(SIGTTOU);
    int rc = tcsetpgrp(esh_sys_tty_getfd(), pgrp);
    if (rc == -1)
	esh_sys_fatal_error("tcsetpgrp: ");

    if (pg_tty_state)
	esh_sys_tty_restore(pg_tty_state);
    esh_signal_unblock(SIGTTOU);
}

/* The shell object plugins use.
 * Some methods are set to defaults.
 */
struct esh_shell shell =
{
    .build_prompt = build_prompt_from_plugins,
    .readline = readline,       /* GNU readline(3) */ 
    .parse_command_line = esh_parse_command_line, /* Default parser */
    .get_jobs = get_jobs,
    .get_job_from_jid = get_job_from_jid,
    .get_job_from_pgrp = get_job_from_pgrp
    /* .get_cmd_from_pid = get_cmd_from_pid */
};

int
main(int ac, char *av[])
{
    int opt;
    int jid = 0;
    list_init(&esh_plugin_list);
    list_init(&current_jobs);

    /* Process command-line arguments. See getopt(3) */
    while ((opt = getopt(ac, av, "hp:")) > 0) {
        switch (opt) {
        case 'h':
            usage(av[0]);
            break;

        case 'p':
            esh_plugin_load_from_directory(optarg);
            break;
        }
    }

    esh_plugin_initialize(&shell);
    setpgid(0, 0);
    struct termios *shell_tty = esh_sys_tty_init();
    give_terminal_to(getpgrp(), shell_tty);

    /*
     * TODO: set up signal handlers here
     */

    /* Read/eval loop. */
    for (;;) {

	/*
	 * TODO: Utilise the functionality given in init_shell for signals and such or we will
	 * not have control returned to us after a job is run.
	 */
	
        /* Do not output a prompt unless shell's stdin is a terminal */
        char * prompt = isatty(0) ? shell.build_prompt() : NULL;
        char * cmdline = shell.readline(prompt);
        free (prompt);
	
        if (cmdline == NULL)  /* User typed EOF */
            break;
	
        struct esh_command_line * cline = shell.parse_command_line(cmdline);
        free (cmdline);
        if (cline == NULL)                  /* Error in command line */
            continue;
	
        if (list_empty(&cline->pipes)) {    /* User hit enter */
            esh_command_line_free(cline);
            continue;
        }

	struct esh_pipeline *pipeline;
	pipeline = list_entry(list_begin(&cline->pipes), struct esh_pipeline, elem);
	
	struct esh_command *commands;
	commands = list_entry(list_begin(&pipeline->commands), struct esh_command, elem);
	//esh_command_print(command);

	int command_type = process_type(commands->argv[0]);

	if (command_type == 1)
	    exit(EXIT_SUCCESS);

	// jobs
	else if (command_type == 2) {

	    char *statusStrings[] = {"Foreground","Background","Stopped", "Needs Terminal"};
	    struct list_elem *e;
	    for (e = list_begin(&current_jobs); e != list_end(&current_jobs); e = list_next(e)) {
		struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);

		printf("[%d] %s \t", pipeline->jid, statusStrings[pipeline->status]);

		struct list_elem *e2;
		for (e2 = list_begin(&pipeline->commands); e2 != list_end(&pipeline->commands); e2 = list_next(e2)) {
		    
		    struct esh_command *command = list_entry(e2, struct esh_command, elem);
		    
		    char **argv = command->argv;
		    while (*argv) {
			printf("%s ", *argv);
			fflush(stdout);
			argv++;
		    }
		    
		    if (list_size(&pipeline->commands) > 1)
			printf("| ");
		}

		printf("\n");
	    }
	}
	
	else if (command_type == 3) {
	    struct list_elem *first_job = list_begin(&current_jobs);
	    
	}

	else if (command_type == 4) {
	    // bg
	}

	else if (command_type == 5) {
	    // kill jid
	}

	else if (command_type == 6) {
	    // stop pid
	}

	else {

	    /*
	     * Don't think this is pipeline friendly.
	     */

	    jid++;
	    pipeline->jid = jid;
	    pipeline->pgrp = -1;
	    pid_t pid;
	    
	    struct list_elem *e;
	    for (e = list_begin(&pipeline->commands); e != list_end(&pipeline->commands); e = list_next(e)) {

		struct esh_command *command = list_entry(e, struct esh_command, elem);
		
		//int status;

		pid = fork();

		// child
		if (pid == 0) {

		    pid = getpid();
		    command->pid = pid;

		    if (pipeline->pgrp == -1)
			pipeline->pgrp = pid;

		    if (setpgid(pid, pipeline->pgrp) < 0)
			esh_sys_fatal_error("Error Setting Process Group");

		    if (!pipeline->bg_job) {
			give_terminal_to(pipeline->pgrp, shell_tty);
			pipeline->status = FOREGROUND;
		    }

		    else
			pipeline->status = BACKGROUND;

		    // signal handling here
		    // piping shit here
		    
		    if (execvp(command->argv[0], command->argv) < 0)
			esh_sys_fatal_error("Exec Error");
		}

		else if (pid < 0) {
		    esh_sys_fatal_error("Fork Error");
		}

		// parent
		else {
		    if (pipeline->pgrp == -1)
			pipeline->pgrp = pid;		    
		    
		    if (setpgid(pid, pipeline->pgrp) < 0)
			esh_sys_fatal_error("Error Setting Process Group");
		}
	    }

	    if (!pipeline->bg_job) {

		/* PUT IN LOOP AND REAP WAIT_ANY INSTEAD OF -1 */
		int status;
		waitpid(-1, &status, WUNTRACED);
		give_terminal_to(getpgrp(), shell_tty);
		
		if (WIFSTOPPED(status)) {
		    pipeline->status = STOPPED;
		    struct list_elem *e = list_pop_front(&cline->pipes);
		    list_push_front(&current_jobs, e);
		    printf("\n[%d]+ Stopped \t ", pipeline->jid);
		    print_job_commands(current_jobs);
		}
		
		if (WTERMSIG(status))
		    printf("\n");
	    }

	    // this is a background job that is currnetly running in the bg with no terminal access
	    else {
		struct list_elem *e = list_pop_front(&cline->pipes);
		list_push_front(&current_jobs, e);		
	    }


	    
	    /*
	     * Set pipeline fields
	     * Set each command's struct fields
	     * Add pipelining support and I/O redirection
	     */
	}

	


	
	//printf("%s\n", command->argv[0]);




	//esh_command_line_print(cline);
        esh_command_line_free(cline);
    }
    return 0;
    
}

/*
 * Checks if the command passed can be handled by the esh shell.
 * If it is not a builtin command, returns 0.
 */
int process_type(char *command)
{
    if (!strcmp(command, "exit"))
	return 1;
	
    else if (!strcmp(command, "jobs"))
	return 2;
    
    else if (!strcmp(command, "fg"))
	return 3;
	
    else if (!strcmp(command, "bg"))
	return 4;
    
    else if (!strcmp(command, "kill"))
	return 5;

    else if (!strcmp(command, "stop"))
	return 6;
	
    return 0;
}

/*
* Executes all pipelines included in esh_command_line.

static void execute_command_line(struct esh_command_line *command_line)
{
	struct list_elem * e = list_begin (&command_line->pipes); 

    for (; e != list_end (&command_line->pipes); e = list_next (e)) 
	{
        struct esh_pipeline *pipe = list_entry(e, struct esh_pipeline, elem);

		//execute the pipeline
		esh_execute_pipe(pipe);
    }
    }*/

/*
* Remove a job from the job list pipeline.

static void remove_job(struct esh_pipeline *pipeline)
{
	struct list_elem *e = list_begin(&jobs_list);
		
	for(; e != list_end(&jobs_list); e = list_next(e))
	{
		struct job *j = list_entry(e, struct job, elem);
			
		if(j->pipe->pgrp == pipeline->pgrp)
		{
			list_remove(e);
			return;
		}
	}
	return;
}
*/
