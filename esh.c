/*
 * esh - the 'pluggable' shell.
 *
 * Developed by Godmar Back for CS 3214 Fall 2009
 * Virginia Tech.
 */
#include <stdio.h>
#include <readline/readline.h>
#include <unistd.h>
#include "esh.h"

//static void execute_command_line (struct esh_command_line *command_line);
//static int  esh_commands(char **argv);
//static void remove_job(struct esh_pipeline *pipe);

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
    return current_jobs;
}

/*
 * Searches the pipeline and returns the job corresponding to jid
 */
static struct esh_pipeline * get_job_from_jid(int jid)
{
    // search pipeline and return job with jid
}

/*
 * Searches the pipeline and return the job corresponding to pgrp
 */
static struct esh_pipeline * get_job_from_pgrp(pid_t pgrp)
{
    // search pipeline and return job from pgrp
}

/*
 * Searches the commands (processes) and returns the process
 * corresponding to pid
 */
static struct esh_command * get_cmd_from_pid(pid_t pid)
{
    //
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
    .get_job_from_pgrp = get_job_from_pgrp,
    .get_cmd_from_pid = get_cmd_from_pid
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

	/*
	 * This assumes we only have one pipeline (job). Must be accomodated
	 * in a for-loop for pipelining / I/O redirection support.
	 */

	struct esh_pipeline *pipeline;
	pipeline = list_entry(list_begin(&cline->pipes), struct esh_pipeline, elem);
	//esh_pipeline_print(pipeline);

	printf("Job ID is %d\n", pipeline->jid);
	
	struct esh_command *command;
	command = list_entry(list_begin(&pipeline->commands), struct esh_command, elem);
	//esh_command_print(command);

	int command_type = process_type(command->argv[0]);

	if (command_type == 1)
	    exit(EXIT_SUCCESS);

	else if (command_type == 2) {
	    // print out list of jobs
	}

	else if (command_type == 3) {
	    // fg
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

	    ++jid;
	    
	    
	    // fork here
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
