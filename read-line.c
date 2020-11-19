/*
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#define MAX_BUFFER_LINE 2048

extern void tty_raw_mode(void);
extern void reset(void);

// Buffer where line is stored
char line_buffer[MAX_BUFFER_LINE];
int line_length;
int temp;

// Simple history array
// This history gets updated as you read in each argument in the buffer. 

char * history [1089];
int history_index = 0;
int history_length = 0;

// Helper function to print the read line usgae 
void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?                  Print usage\n"
    " Up Arrow                Shows the previous command in the history list\n"
    " Down Arrow              Shows the next command in the history list\n"
    " Delete (ctrl-D)         Removes the character at the cursor\n"
    " Backspace (ctrl-H)      Removes the character at the position before the cursor\n"
    " Home (ctrl-A)           The cursor moves to the beginning of the line\n"
    " End (ctrl-E)            The cursor moves to the end of the line\n";

  write(1, usage, strlen(usage));
}


// Helper Function to print the current history array
void print_array(FILE * fp)
{
  // Read line
  fprintf(fp, "Line \"%s\"\n", line_buffer);
  fprintf(fp, "Length \"%d\"\n", line_length);

  fprintf(fp, "History\n");
  for (int i = 0; i < history_length; i++) {
    fprintf(fp, "%d: \"%s\"\n", i, history[i]);
  }
  fprintf(fp, "History Length \"%d\"\n", history_length);
  fprintf(fp, "History Index \"%d\"\n", history_index);

  fprintf(fp, "\n");
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {
  // FILE * fp = fopen("out.txt", "a");

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  temp = line_length;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch == 8 || ch == 127) {
      // <backspace> was typed. Remove previous character read and shift the characters
      // 8 i.e ctrl H or delete key

      if (line_length > 0) {
          int count = 0; 

          // Keep moving the cursor back until you reach the start of the buffer
          while(temp > 0) {
            ch = 8;
            write(1,&ch,1);
            temp--;	
            count++;	
          }

          int cursor = count - 1;

          // Remove the character from the line buffer
          // Shift each character after "count" to the left
          for (int i = count; i < line_length; i++) {
            line_buffer[i - 1] = line_buffer[i];            
          }
          line_buffer[line_length - 1] = '\0';
          line_length--;

          // Resetting temp 
          temp = line_length;

          // Deleting the entire line from screen

          // Moving the cursor to the end and deleting the string
          count = 0;
          while (count != line_length + 1) {
            // Write a space 
            ch = ' ';
            write(1,&ch,1);
            count++;		
          }

          // Rewriting the new string
          while (count > 0){
            ch = 8;
            write(1,&ch,1);
            count--;		
          }

          for (int i = 0; i < line_length; i++) {
            ch = line_buffer[i];
            write(1,&ch,1);
          }

          // Move cursor to the correct position
          while (temp != cursor) {
            ch = 8;
            write(1,&ch,1);
            temp--;
          }
          
        }
      continue;
    }
    else if (ch>=32) {
      // It is a printable character. 

      // Find where the cursor is
      int cursor = 0;
      int count = 0;
      int temp_temp = temp;
      char chx = ch;
      if (line_length > 0) { 
        // Keep moving the cursor back until you reach the start of the buffer
        while(temp_temp > 0) {
          ch = 8;
          write(1,&ch,1);
          temp_temp--;	
          count++;	
        }
        cursor = count - 1;
      }

      if (line_length == 0 || cursor == line_length) {
        // Do echo
        write(1,&ch,1);

        // If max number of character reached return.
        if (line_length == MAX_BUFFER_LINE-2) {
          break; 
        }

        // add char to buffer.
        line_buffer[line_length] = chx;
        if (temp == line_length) {
          line_length++;
        }
        temp++;
        continue;
      }

      // Add the character to the line buffer
      // Shift each character after "count" to the right

      for (int i = line_length; i > count; i--) {
        line_buffer[i] = line_buffer[i - 1];           
      }
      line_buffer[count] = chx;
      //line_buffer[line_length + 2] = '\0';
      // if (temp == line_length) {
      //   line_length++;
      // }
      temp++;
      line_length++;

      // Moving the cursor to the end and deleting the string
      count = 0;
      while (count != line_length + 1) {
        // Write a space 
        ch = ' ';
        write(1,&ch,1);
        count++;		
      }

      // Rewriting the new string
      while (count > 0){
        ch = 8;
        write(1,&ch,1);
        count--;		
      }

      for (int i = 0; i < line_length; i++) {
        ch = line_buffer[i];
        write(1,&ch,1);
      }

      temp_temp = line_length;
      cursor = cursor + 2;
      // Move cursor to the correct position
      while (temp_temp != cursor) {
        ch = 8;
        write(1,&ch,1);
        temp_temp--;
      }
      continue;
    }
    else if (ch==10) {
      // <Enter> was typed. Return line
      
      // Print newline
      write(1,&ch,1);

      break;
    }
    else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0] = 0;
      break;
    }
    else if (ch == 4) {
      // Functionality for delete i.e ctrl D 
      // 4

      if (line_length > 0) {
        int count = 0; 

        // Keep moving the cursor back until you reach the start of the buffer
        while(temp > 0) {
          ch = 8;
          write(1,&ch,1);
          temp--;	
          count++;	
        }

        // Since, we are deleting the char after the cursor
        count++;

        int cursor = count - 1;

        // If the cursor is at the end of the line
        if (cursor == line_length) {
          // Moving cursor to the end
          count = 0;
          while (count != line_length) {
            ch = 27;
            write(1,&ch,1);
            ch = 91;
            write(1,&ch,1);
            ch = 67;
            write(1,&ch,1);
            count++;		
          }
          continue;
        }

        // Remove the character from the line buffer
        // Shift each character after "count" to the left
        for (int i = count; i < line_length; i++) {
          line_buffer[i - 1] = line_buffer[i];            
        }
        line_buffer[line_length - 1] = '\0';
        line_length--;

        // Resetting temp 
        temp = line_length;

        // Deleting the entire line from screen

        // Moving the cursor to the end and deleting the string
        count = 0;
        while (count != line_length + 1) {
          // Write a space 
          ch = ' ';
          write(1,&ch,1);
          count++;		
        }

        // Rewriting the new string
        while (count > 0){
          ch = 8;
          write(1,&ch,1);
          count--;		
        }

        for (int i = 0; i < line_length; i++) {
          ch = line_buffer[i];
          write(1,&ch,1);
        }

        // Move cursor to the correct position
        while (temp != cursor) {
          ch = 8;
          write(1,&ch,1);
          temp--;
        }
        
      }
      continue;
    }
    else if (ch == 1) {
      // Functionality for home key i.e ctrl A 
      // 1

      // Keep moving the cursor back until you reach the start of the buffer
      while(temp > 0){
			  ch = 8;
        write(1,&ch,1);
			  temp--;		
		  }
      continue;
    }
    else if (ch == 5) {
      // Functionality for end key i.e ctrl E
      // 5

      // Keep moving the cursor forward until you reach the end of the buffer
      while(temp != line_length) {
			  ch = 27;
        write(1,&ch,1);
        ch = 91;
        write(1,&ch,1);
        ch = 67;
        write(1,&ch,1);
			  temp++;		
		  }
      continue;
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1; 
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);

      if (ch1 == 91 && ch2 == 65) {        
	      // Up arrow. Print next line in history.
        // 27 91 65

        // Deleting the entire line from screen

        // Moving the cursor to the end and deleting the string
        int count = line_length;

        // Keep moving the cursor back until you reach the start of the buffer
        while(count > 0) {
          ch = 8;
          write(1,&ch,1);
          count--;		
        }

        // Deleting the string
        while (count != line_length) {
          // Write a space 
          ch = ' ';
          write(1,&ch,1);
          count++;		
        }

        // Reset the cursor to the beginning
        count = line_length;
        while(count > 0) {
          ch = 8;
          write(1,&ch,1);
          count--;		
        }

        // Write the previous command / history only if it is present
        
        // If index is out of bounds i.e < 0 wrap it back around
        // Set it to the index of the last command
        if (history_index < 0) {
          history_index = history_length - 1;
        }

        // Copy the string only is it is non empty 
        if (history[history_index]) {
          strcpy(line_buffer, history[history_index]);
        }
        else {
          strcpy(line_buffer, "");
        }

        // Setting the line length
        line_length = strlen(line_buffer);
        history_index = history_index - 1;

        // echo line
        write(1, line_buffer, line_length);
        continue;
    
      } 
      else if (ch1 == 91 && ch2 == 66) {
        // Down arrow. Print previous line in history.
        // 27 91 66

        // Deleting the entire line from screen

        // Moving the cursor to the end and deleting the string
        int count = line_length;

        // Keep moving the cursor back until you reach the start of the buffer
        while(count > 0) {
          ch = 8;
          write(1,&ch,1);
          count--;		
        }

        // Deleting the string
        while (count != line_length) {
          // Write a space 
          ch = ' ';
          write(1,&ch,1);
          count++;		
        }

        // Reset the cursor to the beginning
        count = line_length;
        while(count > 0) {
          ch = 8;
          write(1,&ch,1);
          count--;		
        }	

        // Write the next command / history only if it is present
        
        // If index is out of bounds i.e < 0 setting it to 0
        // Else we move ahead
        if (history_index < 0) {
          history_index = 0;
        }
        else {
          history_index = history_index + 1;
          // If its out of bounds on the end of the array we wrap around to the beginning
					if (history_index > history_length ) {
						history_index = 0;
					}
        }

        // Copy the string only is it is non empty 
        if (history[history_index]) {
          strcpy(line_buffer, history[history_index]);
        }
        else {
          strcpy(line_buffer, "");
        }

        // Setting the line length
        line_length = strlen(line_buffer);
        // history_index = history_index - 1;

        // echo line
        write(1, line_buffer, line_length);
        continue;
      } 
      else if (ch1 == 91 && ch2 == 67) {
        // Functionality for right arrow 
        // 27 91 67

        // If it's not the end of the line

        if (temp < line_length) {
          ch = 27;
			    write(1,&ch,1);

          ch = 91;
		    	write(1,&ch,1);

          ch = 67;
		    	write(1,&ch,1);

          temp++;
        }
      }
      else if (ch1 == 91 && ch2 == 68) {
        // Functionality for left arrow 
        // 27 91 68

        // If it's not the beginning of the line
        if (temp > 0) {
          ch = 8;
			    write(1,&ch,1);
          temp--;
        }
      }

    } // End main if block

  } // End while


  // Add eol and null char at the end of string
  line_buffer[line_length] = 10;
  line_length++;
  line_buffer[line_length] = 0;

  // Updating the history

  // Malloc for the history table and the temp variable 
  history[history_length] = (char *) malloc(strlen(line_buffer) * sizeof(char) + 1);
  char * line_temp = (char *) malloc(strlen(line_buffer) * sizeof(char) + 1);

  strcpy(line_temp, line_buffer);
  // Insert the NULL terminator
  line_temp[strlen(line_buffer) - 1] = '\0';

  // Insert in the history array
  strcpy(history[history_length], line_temp);
  history_length++;

  // Setting the index
  history_index = history_length - 1;

  // Reset to normal mode
  reset();

  // fclose(fp);
  free(line_temp);
  return line_buffer;
}

