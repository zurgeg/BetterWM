#ifndef AGENT_H
#define AGENT_H

extern int agent_status;
extern int agent_done;
extern int remove_linkkey;

extern char agentcommand[14];
extern char * bdaddr;

void * agent_run(void * arg);

int remove_paired_linkkey();

#endif
