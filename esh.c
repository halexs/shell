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
#include <assert.h>
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

        if (plugin->make_prompt == NULL) {
            continue;
        }

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
    if (prompt == NULL) {
        prompt = strdup("esh> ");
    }

    return prompt;
}

/*
 * Returns the list of current jobs
 */
static struct list * get_jobs(void) {
    return &current_jobs;
}

/*
 * Searches the pipeline and returns the job corresponding to jid
 * Returns the job, or NULL if not found
 */
static struct esh_pipeline * get_job_from_jid(int jid) {
    struct list_elem *e;
    for (e = list_begin(&current_jobs); e != list_end(&current_jobs); e = list_next(e)) {
        struct esh_pipeline *job = list_entry(e, struct esh_pipeline, elem);
        if (job->jid == jid) {
            return job;
        }
    }

    return NULL;
}

/*
 * Searches the pipeline and return the job corresponding to pgrp
 * Returns the job, or NULL if not found
 */
static struct esh_pipeline * get_job_from_pgrp(pid_t pgrp) {
    struct list_elem *e;
    for (e = list_begin(&current_jobs); e != list_end(&current_jobs); e = list_next(e)) {
        struct esh_pipeline *job = list_entry(e, struct esh_pipeline, elem);
        if (job->pgrp == pgrp) {
            return job;
        }
    }

    return NULL;
}

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

    printf("(");
    for (e = list_begin(&pipeline->commands); e != list_end(&pipeline->commands); e = list_next(e)) {
	
        struct esh_command *command = list_entry(e, struct esh_command, elem);
	
        char **argv = command->argv;
        while (*argv) {
            printf("%s ", *argv);
            argv++;
        }

        if (list_size(&pipeline->commands) > 1) {
            printf("| ");
        }
    }

    printf(")\n");
}

static void print_single_job(struct esh_pipeline *pipeline)
{
    printf("(");

    struct list_elem *e;
    for (e = list_begin(&pipeline->commands); e != list_end(&pipeline->commands); e = list_next(e)) {

        struct esh_command *command = list_entry(e, struct esh_command, elem);

        char **argv = command->argv;
        while (*argv) {
            printf("%s ", *argv);
            fflush(stdout);
            argv++;
        }

        if (list_size(&pipeline->commands) > 1) {
            printf("| ");
        }
    }

    printf(")\n");
}

/*
 * Change the status of a given job
 */
static void change_job_status(pid_t pid, int status)
{
    if (pid > 0) {

        struct list_elem *e;
        for (e = list_begin(&current_jobs); e != list_end(&current_jobs); e = list_next(e)) {
	    
            struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);
	    
            if (pipeline->pgrp == pid) {
		
                if (WIFSTOPPED(status)) {
                    pipeline->status = STOPPED;
                    printf("\n[%d]+ Stopped      ", pipeline->jid);
                    print_job_commands(current_jobs);
                }
		
                if (WTERMSIG(status) == 9) {
                    list_remove(e);
                }
		
                else {
                    // printf("\n");
                }
		
                // normal termination
                if (WIFEXITED(status)) {
                    // if job was bg, then print out done stuff (maybe?)
                    list_remove(e);
                }
		
                else if (WIFSIGNALED(status)) {
                    list_remove(e);
                }

                else if (WIFCONTINUED(status)) {
                    list_remove(e);
                }

                if (list_empty(&current_jobs)) {
                    jid = 0;
                }
            }
        }
    }

    else if (pid < 0) {
        esh_sys_fatal_error("Error in wait");
    }
}

/*
 * SIGCHLD Handler
 */
static void child_handler(int sig, siginfo_t *info, void *_ctxt)
{
    assert (sig == SIGCHLD);

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WUNTRACED|WNOHANG)) > 0) {
        change_job_status(pid, status);
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
    if (rc == -1) {
        esh_sys_fatal_error("tcsetpgrp: ");
    }

    if (pg_tty_state) {
        esh_sys_tty_restore(pg_tty_state);
    }
    esh_signal_unblock(SIGTTOU);
}

/*
 * Wait
 */
static void wait_for_job(struct esh_command_line *cline, struct esh_pipeline *pipeline, struct termios *shell_tty, bool is_piped)
{
    int status;
    pid_t pid;

    if (is_piped) {
    
    //    

    while ((pid = waitpid(-1, &status, WUNTRACED)) > 0) {
 
	//if (pid > 0) {
        give_terminal_to(getpgrp(), shell_tty);
        change_job_status(pid, status);
    }
    }

    else {
	pid = waitpid(-1, &status, WUNTRACED);
	if (pid > 0) {
	    give_terminal_to(getpgrp(), shell_tty);
	    change_job_status(pid, status);
	}
    }
	    
}

/* The shell object plugins use.
 * Some methods are set to defaults.
 */
struct esh_shell shell = {
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
    jid = 0;
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

    /* Read/eval loop. */
    for (;;) {

        /* Do not output a prompt unless shell's stdin is a terminal */
        char * prompt = isatty(0) ? shell.build_prompt() : NULL;
        char * cmdline = shell.readline(prompt);
        free (prompt);

        if (cmdline == NULL) { /* User typed EOF */
            break;
        }

        struct esh_command_line * cline = shell.parse_command_line(cmdline);
        free (cmdline);
        if (cline == NULL) {                /* Error in command line */
            continue;
        }

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

        if (command_type == 1) {
            exit(EXIT_SUCCESS);
        }

        // jobs
        else if (command_type == 2) {

            char *statusStrings[] = {"Foreground","Running","Stopped", "Needs Terminal"};
            struct list_elem *e;
            for (e = list_begin(&current_jobs); e != list_end(&current_jobs); e = list_next(e)) {
                struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);
                printf("[%d] %s ", pipeline->jid, statusStrings[pipeline->status]);
                print_single_job(pipeline);
            }
        }

        // fg, bg, kill, stop
        else if (command_type == 3 || command_type == 4 || command_type == 5 || command_type == 6) {

            if (!list_empty(&current_jobs)) {

                int jobid_arg = -1;
		
                if (commands->argv[1] == NULL) {
                    struct list_elem *e = list_back(&current_jobs);
                    struct esh_pipeline *pipeline = list_entry(e, struct esh_pipeline, elem);
                    jobid_arg = pipeline->jid;
                }
                else {
		    if (strncmp(commands->argv[1], "%", 1) == 0) {
			char *temp = (char*) malloc(5);
			strcpy(temp, commands->argv[1]+1);
			jobid_arg = atoi(temp);
			free(temp);
		    } else {
			jobid_arg = atoi(commands->argv[1]);
		    }
                }
		
                struct esh_pipeline *pipeline;
                pipeline = get_job_from_jid(jobid_arg);
		
		// fg
                if (command_type == 3) {
		    
		    esh_signal_block(SIGCHLD);
		    pipeline->status = FOREGROUND;
                    print_single_job(pipeline);
                    give_terminal_to(pipeline->pgrp, shell_tty);

                    // check if SIGCONT is needed in the future -- if (cont) perhaps
                    if (kill (-pipeline->pgrp, SIGCONT) < 0) {
                        esh_sys_fatal_error("fg error: kill SIGCONT");
                    }
		    
                    wait_for_job(cline, pipeline, shell_tty, false);
		    esh_signal_unblock(SIGCHLD);
                }
		
                // bg
                if (command_type == 4) {
		    
                    pipeline->status = BACKGROUND;
		    
                    if (kill(-pipeline->pgrp, SIGCONT) < 0) {
                        esh_sys_fatal_error("SIGCONT Error");
                    }

                    print_job_commands(current_jobs);
                    printf("\n");
                }

                // kill
                else if (command_type == 5) {
                    if (kill(-pipeline->pgrp, SIGKILL) < 0) {
                        esh_sys_fatal_error("SIGKILL Error");
                    }
                }

                // stop
                else if (command_type == 6) {
                    if (kill(-pipeline->pgrp, SIGSTOP) < 0) {
                        esh_sys_fatal_error("SIGSTOP Error");
                    }
                }
            }
        }

        else {

            /*
             * Don't think this is pipeline friendly.
             */

            esh_signal_sethandler(SIGCHLD, child_handler);
	    
	    jid++;
	    if (list_empty(&current_jobs)) {
		jid = 1;
	    }
            pipeline->jid = jid;
            pipeline->pgrp = -1;
            pid_t pid;

	    // piping
	    int process_count = 0;
	    int *mypipe;
	    size_t num_of_pipes;
	    bool is_piped;

	    if (list_size(&pipeline->commands) > 1)
		is_piped = true;
	    else
		is_piped = false;

	    if (is_piped) {

		num_of_pipes = list_size(&pipeline->commands) - 1;

		mypipe = malloc(num_of_pipes * 2 * sizeof(int));
		
		int i;
		for (i = 0; i < num_of_pipes; i++) {	
		    if (pipe(mypipe + (i * 2)) < 0)
			esh_sys_fatal_error("Pipe Error");
		}

		if (pipe(mypipe) < 0)
		    esh_sys_fatal_error("Pipe Error");
	    }
	    
            struct list_elem *e;
            for (e = list_begin(&pipeline->commands); e != list_end(&pipeline->commands); e = list_next(e)) {
		
                struct esh_command *command = list_entry(e, struct esh_command, elem);

                esh_signal_block(SIGCHLD);
                pid = fork();

                // child
                if (pid == 0) {

                    pid = getpid();
                    command->pid = pid;

                    if (pipeline->pgrp == -1) {
                        pipeline->pgrp = pid;
                    }

                    if (setpgid(pid, pipeline->pgrp) < 0) {
                        esh_sys_fatal_error("Error Setting Process Group");
                    }

                    if (!pipeline->bg_job) {
                        give_terminal_to(pipeline->pgrp, shell_tty);
                        pipeline->status = FOREGROUND;
                    }

                    else
                        pipeline->status = BACKGROUND;

		    if (is_piped) {

			// if not first process in the pipeline
			if (e != list_begin(&pipeline->commands)) {
			    if (dup2(mypipe[2 * process_count - 2], 0) < 0)
				esh_sys_fatal_error("dup2  error");
			    close(mypipe[2 * process_count - 1]);
			    close(mypipe[2 * process_count - 2]);
			}

			// if not the last process in the pipeline
			else if (e != list_end(&pipeline->commands)) {
			    close(mypipe[2 * process_count]);
			    if (dup2(mypipe[2 * process_count + 1], 1) < 0)
				esh_sys_fatal_error("dup2 error");
			    close(mypipe[2 * process_count + 1]);
			}

			int i;
			for(i = 0; i < num_of_pipes * 2; i++)
			    close(mypipe[i]);
		    }

                    if (execvp(command->argv[0], command->argv) < 0)
                        esh_sys_fatal_error("Exec Error");
                }

                else if (pid < 0)
                    esh_sys_fatal_error("Fork Error");

                // parent
                else {
		    
                    if (pipeline->pgrp == -1)
                        pipeline->pgrp = pid;

                    if (setpgid(pid, pipeline->pgrp) < 0)
                        esh_sys_fatal_error("Error Setting Process Group");

		    // if not first process in the pipeline
		    if (e != list_begin(&pipeline->commands)) {
			close(mypipe[2*process_count-1]);
			close(mypipe[2*process_count-2]);
		    }
                }

		process_count++;
            }
	    
            if (pipeline->bg_job) {
                pipeline->status = BACKGROUND;
                printf("[%d] %d\n", pipeline->jid, pipeline->pgrp);
            }

            e = list_pop_front(&cline->pipes);
            list_push_back(&current_jobs, e);

            if (!pipeline->bg_job)
                wait_for_job(cline, pipeline, shell_tty, is_piped);

	    if (is_piped)
		free(mypipe);

            esh_signal_unblock(SIGCHLD);
        }

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
    if (!strcmp(command, "exit")) {
        return 1;
    }

    else if (!strcmp(command, "jobs")) {
        return 2;
    }

    else if (!strcmp(command, "fg")) {
        return 3;
    }

    else if (!strcmp(command, "bg")) {
        return 4;
    }

    else if (!strcmp(command, "kill")) {
        return 5;
    }

    else if (!strcmp(command, "stop")) {
        return 6;
    }

    return 0;
}
