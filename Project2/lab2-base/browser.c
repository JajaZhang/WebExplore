/* CSci4061 F2016 Assignment 2
* date: 10/27/16
* name: Jason Zhang, Chris Fan
* id: 4701656(zhan3507), 4903718(fanxx332)*/
#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_TAB 100
#define BUF_SIZE 16
extern int errno;

/*
 * Name:                uri_entered_cb
 * Input arguments:     'entry'-address bar where the url was entered
 *                      'data'-auxiliary data sent along with the event
 * Output arguments:    none
 * Function:            When the user hits the enter after entering the url
 *                      in the address bar, 'activate' event is generated
 *                      for the Widget Entry, for which 'uri_entered_cb'
 *                      callback is called. Controller-tab captures this event
 *                      and sends the browsing request to the ROUTER (parent)
 *                      process.
 */
void uri_entered_cb(GtkWidget* entry, gpointer data) {
	if(data == NULL) {	
		return;
	}
	browser_window *b_window = (browser_window *)data;
	// This channel has pipes to communicate with ROUTER.
	comm_channel channel = b_window->channel;
	// Get the tab index where the URL is to be rendered
	int tab_index = query_tab_id_for_request(entry, data);
	if(tab_index <= 0) {
		return;
	}
	// Get the URL.
	char * uri = get_entered_uri(entry);
	child_req_to_parent msgURI;
	msgURI.type = NEW_URI_ENTERED;
	msgURI.req.uri_req.render_in_tab =tab_index;
	strcpy(msgURI.req.uri_req.uri, uri);
	write(channel.child_to_parent_fd[1], &msgURI, sizeof(msgURI));
	return;
	// Append your code here
	
	// ------------------------------------
	// * Prepare a NEW_URI_ENTERED packet to send to ROUTER (parent) process.
	// * Send the url and tab index to ROUTER
	// ------------------------------------
}

/*
 * Name:                create_new_tab_cb
 * Input arguments:     'button' - whose click generated this callback
 *                      'data' - auxillary data passed along for handling
 *                      this event.
 * Output arguments:    none
 * Function:            This is the callback function for the 'create_new_tab'
 *                      event which is generated when the user clicks the '+'
 *                      button in the controller-tab. The controller-tab
 *                      redirects the request to the ROUTER (parent) process
 *                      which then creates a new child process for creating
 *                      and managing this new tab.
 */ 
void create_new_tab_cb(GtkButton *button, gpointer data) {
	if(data == NULL) {
		return;
	}
	// This channel has pipes to communicate with ROUTER. 
	// Append your code here.
	// ------------------------------------
	// * Send a CREATE_TAB message to ROUTER (parent) process.
	// ------------------------------------
	comm_channel channel = ((browser_window*)data)->channel;
	child_req_to_parent msgCreate;
	msgCreate.type = CREATE_TAB;
	write(channel.child_to_parent_fd[1], &msgCreate, sizeof(msgCreate));
	return;
	
}

/*
 * Name:                url_rendering_process
 * Input arguments:     'tab_index': URL-RENDERING tab index
 *                      'channel': Includes pipes to communctaion with
 *                      Router process
 * Output arguments:    none
 * Function:            This function will make a URL-RENDRERING tab Note.
 *                      You need to use below functions to handle tab event. 
 *                      1. process_all_gtk_events();
 *                      2. process_single_gtk_event();
*/
int url_rendering_process(int tab_index, comm_channel *channel) {
	// Don't forget to close pipe fds which are unused by this process
	//sleep(20);
	int ppid;
	ppid = getppid();
	close(channel->child_to_parent_fd[0]);
	close(channel->parent_to_child_fd[1]);
	browser_window * b_window = NULL;
	// Create url-rendering window
	create_browser(URL_RENDERING_TAB, tab_index, G_CALLBACK(create_new_tab_cb), G_CALLBACK(uri_entered_cb), &b_window, channel);
	child_req_to_parent msg;
	while (1) {
		if (getppid() != ppid){
			exit(0);
		}
		// Handle one gtk event, you don't need to change it nor understand what it does.
		process_single_gtk_event();		
		// Poll message from ROUTER
		// It is unnecessary to poll requests unstoppably, that will consume too much CPU time
		// Sleep some time, e.g. 1 ms, and render CPU to other processes
		usleep(1000);
		// Append your code here
		if (read(channel->parent_to_child_fd[0], &msg, sizeof(msg)) != -1){
			if (msg.type == CREATE_TAB){
				continue;
			}
			else if (msg.type == NEW_URI_ENTERED){  // if msg is new uri entered 
				render_web_page_in_tab(msg.req.uri_req.uri, b_window);
			}
			else if (msg.type == TAB_KILLED){
				process_all_gtk_events();
				exit(0);
			}
			else{
			}
		}
		
		
		// Try to read data sent from ROUTER
		// If no data being read, go back to the while loop
		// Otherwise, check message type:
		//   * NEW_URI_ENTERED
		//     ** call render_web_page_in_tab(req.req.uri_req.uri, b_window);
		//   * TAB_KILLED
		//     ** call process_all_gtk_events() to process all gtk events and jump out of the loop
		//   * CREATE_TAB or unknown message type
		//     ** print an error message and ignore it
		// Handle read error, e.g. what if the ROUTER has quit unexpected?
	}
	return 0;
}
/*
 * Name:                controller_process
 * Input arguments:     'channel': Includes pipes to communctaion with
 *                      Router process
 * Output arguments:    none
 * Function:            This function will make a CONTROLLER window and 
 *                      be blocked until the program terminates.
 */
int controller_process(comm_channel *channel) {
	// Do not need to change code in this function
	close(channel->child_to_parent_fd[0]);
	close(channel->parent_to_child_fd[1]);
	browser_window * b_window = NULL;
	// Create controler window
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(create_new_tab_cb), G_CALLBACK(uri_entered_cb), &b_window, channel);
	show_browser();
	return 0;
}

/*
 * Name:                router_process
 * Input arguments:     none
 * Output arguments:    none
 * Function:            This function implements the logic of ROUTER process.
 *                      It will first create the CONTROLLER process  and then
 *                      polling request messages from all ite child processes.
 *                      It does not need to call any gtk library function.
 */
int router_process() {
	comm_channel *channel[MAX_TAB];
	int tab_pid_array[MAX_TAB] = {0}; // You can use this array to save the pid
	                                  // of every child process that has been
					  // created. When a chile process receives
					  // a TAB_KILLED message, you can call waitpid()
					  // to check if the process is indeed completed.
					  // This prevents the child process to become
					  // Zombie process. Another usage of this array
					  // is for bookkeeping which tab index has been
					  // taken.
	
	int pid, pipeCreate, i, status, readText;	
	for (i = 0; i < MAX_TAB; i++){
		channel[i] =  malloc(sizeof(channel));
	}		  
	for (i = 0; i < MAX_TAB; i++){
		tab_pid_array[i] = 0;
	}
	// Append your code here
	if ((pipeCreate = pipe(channel[0]->parent_to_child_fd)) == -1){  // set fork and pipe controller
		printf("Fail to create pipe for controller parent_to_child \n");
		exit(EXIT_FAILURE);
	}
	fcntl( channel[0]->parent_to_child_fd[0], F_SETFL, fcntl(channel[0]->parent_to_child_fd[0], F_GETFL) | O_NONBLOCK); //none blocking pipe
	if ((pipeCreate = pipe(channel[0]->child_to_parent_fd)) == -1){
		printf("Fail to create pipe for controller child_to_parent \n");
		exit(EXIT_FAILURE);
	}
	fcntl( channel[0]->child_to_parent_fd[0], F_SETFL, fcntl(channel[0]->child_to_parent_fd[0], F_GETFL) | O_NONBLOCK); //none blocking pipe
	child_req_to_parent killmsg;
	killmsg.type = TAB_KILLED;

	child_req_to_parent dynamicmsg; 
	
	if ((pid = fork()) > 0){  // router_process
		close(channel[0]->parent_to_child_fd[0]);  // close child to parent in parent process   can write from parent to child, can read from child to parent
		close(channel[0]->child_to_parent_fd[1]);  
		tab_pid_array[0] = pid;
		while (1){
			usleep(1000);
			int flagTerminate = 1;
			for (i = 0; i < MAX_TAB; i++){   // this is the main reading loop, it checks controller and the rest tab
				if (i == 0) {  // this is checking controller process
					if (( waitpid(tab_pid_array[0], &status, WNOHANG) != 0) || (tab_pid_array[0] == 0) ) { // test if controller is dead or id no longer exists in form of zombie
						tab_pid_array[0] = 0;
						printf("Controller was closed\n");
						int temp;
						for (temp = 1; temp < MAX_TAB; temp++){   // clean up all child processes
							if (tab_pid_array[temp] != 0){  // if sees a child URL process id exists in the array => process still alive
								write(channel[temp]->parent_to_child_fd[1],  &killmsg, sizeof(killmsg)  );  // send kill
								if (waitpid(tab_pid_array[temp], &status, 0) == -1){  // wait and collect zombie
									printf("fail to wait for child\n");
								}
								else{
									tab_pid_array[temp] = 0;
								}
							}
						}
						return 0;
					}
					else{ // controller alive
						flagTerminate = 0; //the controller is not terminated
						while ((readText = read(channel[0]->child_to_parent_fd[0], &dynamicmsg, sizeof(dynamicmsg))) != -1){  // check if it can read anything
							if (dynamicmsg.type == CREATE_TAB) {  // if read "tab create" request
								int newIndex, flagCreate;
								flagCreate = 0;
								for (newIndex = 0; newIndex < MAX_TAB; newIndex++){  //find next available process spot
									if (tab_pid_array[newIndex] == 0){
										if (pipe(channel[newIndex]->parent_to_child_fd) == -1){
											printf("Fail to create the pipe between router and process at index %d\n", newIndex);
										}
										else fcntl(channel[newIndex]->parent_to_child_fd[0], F_SETFL, fcntl(channel[newIndex]->parent_to_child_fd[0], F_GETFL) | O_NONBLOCK);
										if (pipe(channel[newIndex]->child_to_parent_fd) == -1){
											printf("Fail to create the pipe between router and process at index %d\n", newIndex);
										}
										else fcntl(channel[newIndex]->child_to_parent_fd[0], F_SETFL, fcntl(channel[newIndex]->child_to_parent_fd[0], F_GETFL) | O_NONBLOCK);
										
										flagCreate = 1; // flag checking if a tab is actually created
										if ((pid = fork()) > 0){ // router
											close(channel[newIndex]->parent_to_child_fd[0]);  // close child to parent in parent process   can write from parent to child, can read from child to parent
											close(channel[newIndex]->child_to_parent_fd[1]);  
											tab_pid_array[newIndex] = pid;
											printf("create pid %d\n", pid);
											break;
										}
										else if (pid == 0) {  //url
											url_rendering_process(newIndex, channel[newIndex]);    //url_rendering_process(int tab_index, comm_channel *channel)  
											exit(0);
										}
										else{ //error
											printf("Fail to fork more tab \n");
										}
									}
								}
								if (!flagCreate) {  //if no tab created, the array has reached max
									printf("Maximum tab reached \n");
								}	
							}
							else if (dynamicmsg.type == NEW_URI_ENTERED) {   // if read "new url" request
								//printf("new url %d\n", dynamicmsg.req.uri_req.render_in_tab);
								if (tab_pid_array[dynamicmsg.req.uri_req.render_in_tab] == 0){  // in case if the target child is dead
									printf("Tab is closed\n");
								}
								else{
									if (write(channel[dynamicmsg.req.uri_req.render_in_tab] -> parent_to_child_fd[1], &dynamicmsg, sizeof(dynamicmsg)) == -1){ // write the url message to the child channel
										printf("Tab %d hasn't been created \n", dynamicmsg.req.uri_req.render_in_tab);
									}
								}
							}
							else if (dynamicmsg.type == TAB_KILLED){  // if read "kill" request from controller
								tab_pid_array[0]= 0;
								int temp;
								for (temp = 1; temp < MAX_TAB; temp++){   // clean up all child processes
									if (tab_pid_array[temp] != 0){  // if sees a child URL process
										write(channel[temp]->parent_to_child_fd[1],  &killmsg, sizeof(killmsg));// write the kill message to the child
										if (waitpid(tab_pid_array[temp], &status, 0) == -1){
											printf("fail to wait for child\n");
										}
										else{
											//printf("I caught this process %d\n", temp);
											tab_pid_array[temp] = 0;
										}
									}
								}
								waitpid(tab_pid_array[0], &status, 0); // collect controller
								return 0;
							}
							else{
								printf("Unexpected message type from controller\n");
							} 
						}
					}			 		
				}
				else{
					if (tab_pid_array[i] != 0){
						if (waitpid(tab_pid_array[i], &status, WNOHANG) != 0) { // if the child process is killed expectedly
							tab_pid_array[i] = 0; //clean the zombie and set array index to be zero, mark it complete gone
						}
						else{
							while (read(channel[i] -> child_to_parent_fd[0], &dynamicmsg, sizeof(dynamicmsg))!=-1){
								if (dynamicmsg.type == TAB_KILLED){  // if read tab_killed message
									int targetIndex = dynamicmsg.req.killed_req.tab_index;
									write(channel[targetIndex]->parent_to_child_fd[1],  &dynamicmsg, sizeof(dynamicmsg)  );  // write kill
									//printf("I want to kill index %d\n", targetIndex);
									if (waitpid(tab_pid_array[targetIndex], &status, 0) == -1){  // collect its zombie process
										printf("fail to wait for child\n");
									}
									else{
										//printf("I caught this process %d\n", targetIndex);
										tab_pid_array[targetIndex] = 0;  // reset process array ID open
										break;
									}
								}
								
							}		
						}
					}
				}
			}
			if (flagTerminate){
				return 0;
			}
		}
	}
	else if (pid == 0){  // controller_process
		controller_process(channel[0]);		
		exit(EXIT_SUCCESS);
	}
	else{
		printf("Fail at fork \n");
		exit(EXIT_FAILURE);
	}

	// Prepare communication pipes with the CONTROLLER process
	// Fork the CONTROLLER process
	//   call controller_process() in the forked CONTROLLER process
	// Don't forget to close pipe fds which are unused by this process
	// Poll child processes' communication channels using non-blocking pipes.
	// Before any other URL-RENDERING process is created, CONTROLLER process
	// is the only child process. When one or more URL-RENDERING processes
	// are created, you would also need to poll their communication pipe.
	//   * sleep some time if no message received
	//   * if message received, handle it:
	//     ** CREATE_TAB:
	//
	//        Prepare communication pipes with the new URL-RENDERING process
	//        Fork the new URL-RENDERING process
	//
	//     ** NEW_URI_ENTERED:
	//
	//        Send TAB_KILLED message to the URL-RENDERING process in which
	//        the new url is going to be rendered
	//
	//     ** TAB_KILLED:
	//
	//        If the killed process is the CONTROLLER process
	//        *** send TAB_KILLED messages to kill all the URL-RENDERING processes
	//        *** call waitpid on every child URL-RENDERING processes
	//        *** self exit
	//
	//        If the killed process is a URL-RENDERING process
	//        *** send TAB_KILLED to the URL-RENDERING
	//        *** call waitpid on every child URL-RENDERING processes
	//        *** close pipes for that specific process
	//        *** remove its pid from tab_pid_array[]
	//
	

	return 0;
}

int main() {
	return router_process();
}
		//maybe combine killing controller
		// how does children process know if router dead
//http://www.google.com
