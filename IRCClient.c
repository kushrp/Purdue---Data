#include <stdio.h>
#include <time.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <gtk/gtk.h>

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LEN 300
#define MAX_RESPONSE (20 * 1024)

char responsepls[MAX_RESPONSE];

int lastMessage = 0;

char * host = "localhost";
char * user = "superman";
char * password = "clarkkent";
char * sport;
int port = 2400;

char * roomname;
char * prevname;

GtkWidget *tree_view;
GtkTreeSelection *selection;

GtkListStore * list_rooms;
GtkListStore * list_names;




/*
			           DO NOT TOUCH 
	___________________________________________________________________________________
	___________________________________________________________________________________
	___________________________________________________________________________________

 */

/* Create the list of "messages" */
static GtkWidget *create_list( const char * titleColumn, GtkListStore *model )
{
    GtkWidget *scrolled_window;
    //GtkListStore *model;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    //model = gtk_list_store_new (1, G_TYPE_STRING);
    tree_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view);
   
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view),
	  		         GTK_TREE_VIEW_COLUMN (column));

    return scrolled_window;
}
   
/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */

static void insert_text( GtkTextBuffer *buffer, const char * initialText )
{
   GtkTextIter iter;
 
   gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
   gtk_text_buffer_insert (buffer, &iter, initialText,-1);
}
   
/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_text( const char * initialText )
{
   GtkWidget *scrolled_window;
   GtkWidget *view;
   GtkTextBuffer *buffer;

   view = gtk_text_view_new ();
   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		   	           GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

   gtk_container_add (GTK_CONTAINER (scrolled_window), view);
   insert_text (buffer, initialText);

   gtk_widget_show_all (scrolled_window);

   return scrolled_window;
}

int open_client_socket(char * host, int port) {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		perror("gethostbyname");
		exit(1);
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		perror("getprotobyname");
		exit(1);
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress,
		    sizeof(socketAddress)) < 0) {
		perror("connect");
		exit(1);
	}
	
	return sock;
}

int sendCommand(char * host, int port, char * command, char * user,
		char * password, char * args, char * response) {
	int sock = open_client_socket( host, port);

	// Send command
	write(sock, command, strlen(command));
	write(sock, " ", 1);
	write(sock, user, strlen(user));
	write(sock, " ", 1);
	write(sock, password, strlen(password));
	write(sock, " ", 1);
	write(sock, args, strlen(args));
	write(sock, "\r\n",2);

	// Keep reading until connection is closed or MAX_REPONSE
	int n = 0;
	int len = 0;
	while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
		len += n;
	}

	//printf("response:%s\n", response);

	close(sock);
}

void printUsage()
{
	printf("Usage: talk-client host port user password\n");
	exit(1);
}


/*
					ALL USEFUL FUNCTIONS START HERE
 ___________________________________________________________________________________________________
 ___________________________________________________________________________________________________
 ___________________________________________________________________________________________________ 

*/

static void loginwindow(GtkWidget *widget, GtkWindow *data) {
	GtkWidget *window, *table;
	GtkWidget *entryu, *entryp;
	GtkWidget *labelu, *labelp;
	gint response;
	
	window = gtk_dialog_new_with_buttons ("Encrypted Login Window", NULL, GTK_DIALOG_MODAL, GTK_STOCK_OK, 		GTK_RESPONSE_OK, GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(window), GTK_RESPONSE_OK);

    labelu = gtk_label_new("Username: ");
    labelp = gtk_label_new("Password: ");
    entryu = gtk_entry_new();
    entryp = gtk_entry_new();

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_attach_defaults(GTK_TABLE (table), labelu, 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE (table), labelp, 0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE (table), entryu, 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE (table), entryp, 1, 2, 1, 2);

    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_box_pack_start_defaults(GTK_BOX  (GTK_DIALOG (window)->vbox), table);

    gtk_widget_show_all(window);
    response = gtk_dialog_run (GTK_DIALOG(window));
    if(response == GTK_RESPONSE_OK) {
        //g_print("The username is: %s\n", gtk_entry_get_text (GTK_ENTRY (entryu)));
        //g_print("The password is: %s\n", gtk_entry_get_text (GTK_ENTRY (entryp)));
        user = (char *)gtk_entry_get_text (GTK_ENTRY (entryu));
        password = (char *)gtk_entry_get_text (GTK_ENTRY (entryp));
        char response[ MAX_RESPONSE ];
		sendCommand(host, port, "ADD-USER", user, password, "", response);
	
		if (!strcmp(response,"OK\r\n")) printf("User %s added\n", user);
    }
	gtk_widget_destroy(window);
}


void update_list_rooms() {
    GtkTreeIter iter;
	printf("1\n");
	gtk_list_store_clear(GTK_LIST_STORE (list_rooms));
	printf("2\n");
	char response[MAX_RESPONSE];
	sendCommand(host, port, "LIST-ROOMS", user, password, "", response);
	char * token = strtok(response,"\r\n");
    while(token != NULL) {
		gchar *msg = g_strdup((gchar *)token);
        gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
        gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iter, 0, msg, -1);
		//g_free (msg);
		token = strtok(NULL, "\r\n");
    }
}

void update_list_names(char * Rname) {
	GtkTreeIter iter;
	if(strcmp(Rname,"No room selected") == 0) 
	{
		gtk_list_store_clear(GTK_LIST_STORE (list_names));
		gchar *msg = g_strdup("No room selected");
		gtk_list_store_append (GTK_LIST_STORE (list_names), &iter);
		gtk_list_store_set (GTK_LIST_STORE (list_names), &iter, 0, msg, -1);
		//g_free (msg);
	}
    else {
		//GtkTreeIter iter;
		gtk_list_store_clear(GTK_LIST_STORE (list_names));
		char response[MAX_RESPONSE];
		sendCommand(host, port, "GET-USERS-IN-ROOM", user, password, Rname, response);
		printf("Response Get-users-in-room: %s\n",response);
		char * token = strtok(response,"\r\n");
    	while(token != NULL) {
			gchar *msg = g_strdup((gchar *)token);
        	gtk_list_store_append (GTK_LIST_STORE (list_names), &iter);
        	gtk_list_store_set (GTK_LIST_STORE (list_names), &iter, 0, msg, -1);
			g_free (msg);
			token = strtok(NULL, "\r\n");
    	}
	}
}

void on_changed(GtkWidget *widget, gpointer label) 
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  //char *value;
  if(prevname !=NULL){
	 prevname = roomname;
	 char response[MAX_RESPONSE];
	 sendCommand(host, port, "LEAVE-ROOM", user, password, prevname, responsepls);
	 printf("Response Leave room: %s\n",responsepls);
  }
	

  if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
    gtk_tree_model_get(model, &iter, 0, &roomname,  -1);
    gtk_label_set_text(GTK_LABEL(label), roomname);
    //g_free(value);
  }
	//printf("%s\n",value);
	char response[MAX_RESPONSE];
	sendCommand(host, port, "ENTER-ROOM", user, password, roomname, responsepls);
	printf("Response Enter Room: %s\n",responsepls);
	update_list_names(roomname);
	prevname = roomname;
}
	





static void create_room (GtkWidget *widget, GtkWidget *entry ) {
	const gchar *entry_text;
    entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
    printf ("Entry contents: %s\n", entry_text);
   // user = (char *)entry_text;

	char response[MAX_RESPONSE];
	sendCommand(host, port, "CREATE-ROOM", user, password, (char *)entry_text, response);
	printf("Response Create Room: %s\n",response);
	update_list_rooms();
}

static void create_room1 (GtkWidget *widget, GtkWidget *entry ) {}

int main(int argc, char *argv[] )
{
    GtkWidget *window;
    GtkWidget *list;
	GtkWidget *list2;
    GtkWidget *messages;
    GtkWidget *myMessage;

	GtkWidget *R_entry;
	GtkWidget *croom;
	GtkWidget *cacc;
	GtkWidget *text;

	prevname = NULL;

    gtk_init (&argc, &argv);
   
	// Window
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "IRCClient");
    g_signal_connect (window, "destroy",
	              G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window), 800, 650);



    // Table 10x10
    GtkWidget *table = gtk_table_new (10, 10, TRUE);
    gtk_container_add (GTK_CONTAINER (window), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 10);
    gtk_table_set_col_spacings(GTK_TABLE (table), 10);
    gtk_widget_show (table);



    // list_rooms LIST
	text = gtk_label_new("");
    list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
    update_list_rooms();
    list = create_list ("Rooms", list_rooms);
    gtk_table_attach_defaults (GTK_TABLE (table), list, 0, 3, 0, 4);
    gtk_widget_show (list);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	g_signal_connect(selection, "changed", G_CALLBACK(on_changed), text);


	//list_names LIST
	list_names = gtk_list_store_new (1, G_TYPE_STRING);
    update_list_names("No room selected");
    list2 = create_list ("Users in Room", list_names);
    gtk_table_attach_defaults (GTK_TABLE (table), list2, 0, 3, 6, 10);
    gtk_widget_show (list2);
	


   
	// Room ENTRY
	R_entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (R_entry), 15);
    g_signal_connect (R_entry, "activate", G_CALLBACK (create_room1), R_entry);
    gtk_entry_set_text (GTK_ENTRY (R_entry), "Room name");	
	gtk_table_attach_defaults(GTK_TABLE (table), R_entry, 0, 3, 4, 5); 
    gtk_widget_show (R_entry);




	// Create Room BUTTON
    croom = gtk_button_new_with_label ("Create Room");
    gtk_table_attach_defaults(GTK_TABLE (table), croom, 0, 3, 5, 6); 
    gtk_widget_show (croom);
	g_signal_connect (croom, "clicked", G_CALLBACK (create_room), R_entry);



	// Create account BUTTON
	cacc = gtk_button_new_with_label ("Create Account / Sign In");
    gtk_table_attach_defaults(GTK_TABLE (table), cacc, 7, 10, 5, 6); 
    gtk_widget_show (cacc);
	g_signal_connect (cacc, "clicked", G_CALLBACK (loginwindow), NULL);


	
    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
    messages = create_text ("Peter: Hi how are you\nMary: I am fine, thanks and you?\nPeter: Fine thanks.\n");
    gtk_table_attach_defaults (GTK_TABLE (table), messages, 3, 7, 0, 7);
    gtk_widget_show (messages);



    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
    myMessage = create_text ("I am fine, thanks and you?\n");
    gtk_table_attach_defaults (GTK_TABLE (table), myMessage, 3, 7, 7, 9);
    gtk_widget_show (myMessage);



    // Add send button. Use columns 0 to 1 (exclusive) and rows 4 to 7 (exclusive)
    GtkWidget *send_button = gtk_button_new_with_label ("Send");
    gtk_table_attach_defaults(GTK_TABLE (table), send_button, 3, 7, 9, 10); 
    gtk_widget_show (send_button);


    
    gtk_widget_show (table);
    gtk_widget_show (window);



    gtk_main ();

    return 0;
}

