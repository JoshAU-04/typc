/*
 * Typc - A console-based typing trainer
 * Copyright (C) 2025 Joshua Rose <joshuarose@gmx.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define REGULAR_FILE   DT_REG
#define MAX_PATH_SIZE  (PATH_MAX)
#define ENTRIES_DIR    "/usr/local/lib/typc/texts"

/* scores file (prefixed by $HOME) */
static char *scores_file = ".local/state/typc/data.csv";

/* When in 'tty mode' (teletype mode) where text is scrolling, this is the
 * amount of characters that are shown from the position of the current cursor
 * to the position of the end of the screen.
 */
static int char_offset = 20;

/* Can be modified with --debug */
static int debug = 0;

/* Home dir value. Used for saving to csv files and other stuff. Needs to be
 * static as it's accessed by multiple other static functions */
static char *home_dir;

/* minimal printable char */
#define PRINT_CHAR_MIN 32

/* maximal printable char */
#define PRINT_CHAR_MAX 126

/**
 * Easier visual on error
 *
 * Show character that needs to be typed instead of the character that was typed.
 */
#define HIDE_ERR       true

/* Global flag to control text wrapping mode */
static int wrap_mode = 0;

/**
 * Draw wrapped text. I.e. show all the text that can be possibly shown within
 * the contraints of the width and height of the terminal display. This is with
 * the --wrap option. By default the --wrap option if not toggled on.
 */
static void draw_wrapped(const char *text, int total_chars,
			 int screen_width, char *typed, int current_index);

/**
 * @draw_scrolled - Draw scrolled text.
 *
 * @text - The text to draw.
 * @total_chars - The total number of chars in the text
 * @i - The current index
 * @screen_width - self explainatory
 * @typed - chars already typed. Should be none.
 * @current_index - chars already typed. Should be none.
 *
 */
static void draw_scrolled(const char *text, int total_chars, size_t i,
			  int screen_width, char *typed,
			  int current_index);

/**
 * collect_dir_entries - Collect regular file entries from a directory.
 * @path: Path to the directory.
 * @count: Pointer to a variable to store the number of collected entries.
 *
 * Reads the specified directory, collecting names of regular files.
 * Returns an array of dynamically allocated strings containing file names, or NULL on error.
 * The caller is responsible for freeing the array and each string.
 */
static char **collect_dir_entries(const char *path, size_t *count);

/**
 * calc_speed - Get WPM and CPM values from a file.
 * @filename: Path to the file to read.
 * @elapsed: Total elapsed time.
 * @wpm: Words typed per minute value.
 * @cpm: Characters typed per minute value.
 *
 * This function reads the file specified by 'filename',
 * counts the total non-whitespace characters, and then calculates
 * the characters per minute (CPM) and words per minute (WPM) based on
 * the elapsed time (in seconds).
 */
static void calc_speed(const char *filename, double elapsed, double *wpm,
		       double *cpm);
/*
 * create_or_append() extracts the directory part of 'filepath',
 * creates it (and any missing parent directories), and then opens
 * the file in append mode to write 'info' into it.
 */
static int create_or_append(const char *filepath, const char *info);

/**
 * Creates the file $HOME/.local/state/typc/data.csv, including all parent directories.
 * Returns 0 on success, -1 on failure.
 */
static int create_data_csv(void);

/**
 * random_file_from_dir - Select a random file from the directory.
 *
 * Collects directory entries from the ENTRIES_DIR directory and picks one file randomly.
 * Returns a dynamically allocated string containing the file name, or NULL on error.
 */
static char *random_file_from_dir(void);

/**
 * read_file - Read an entire file into a buffer.
 * @path: Path to the file to read.
 *
 * Opens the file, reads its content into a dynamically allocated buffer,
 * and null-terminates the string. Returns the buffer on success, or NULL on error.
 */
static char *read_file(const char *path);

/**
 * get_random_entry - Select a random entry from a list.
 * @files: Array of file name strings.
 * @count: Number of files in the array.
 *
 * Returns a randomly selected entry from the files array.
 */
static char *get_random_entry(char **files, size_t count);

/**
 * seed_rng - Seed the random number generator.
 *
 * Seeds the random number generator using the current time.
 */
static void seed_rng(void);

/**
 * save_score - Save the typing test score to a file.
 * @wpm: Words per minute.
 * @cpm: Characters per minute.
 * @accuracy: Accuracy in percentage.
 * @consistency: Consistency in percentage.
 * @path: Name of the file path.
 *
 * Opens the scores file (creating it if necessary) and appends a new line containing
 * the WPM, CPM, accuracy, and consistency metrics, separated by commas.
 */
static void save_score(double wpm, double cpm, double accuracy,
		       double consistency, char *path);

/**
 * run_typing_trainer - Run an ncurses-based typing trainer.
 * @path: Path to the file containing the text.
 * @text: The text to be typed by the user.
 *
 * Uses ncurses to display the text for typing.
 * In normal mode, horizontal scrolling is implemented so that the current position remains visible.
 * In wrap mode, the text is wrapped to the screen width.
 * The already typed text is displayed character-by-character:
 * - Correct characters are shown in white.
 * - Incorrect characters are shown in red (or shown as the expected character if HIDE_ERR is true).
 * The untyped text is rendered in a dim (grayish) style.
 * Backspace support allows corrections.
 * Upon completion, the function calculates and displays the words-per-minute (WPM),
 * characters-per-minute (CPM), and the accuracy metric.
 */
static void run_typing_trainer(char *path, const char *text);

/**
 * usage - Print program usage.
 * @progname: The program name.
 *
 * Sends the program usage details to stderr.
 */
static void usage(char *progname);

/**
 * main - Entry point for the typing trainer program.
 *
 * Reads a random text file from the ENTRIES_DIR directory and launches the typing trainer.
 * A "--wrap" argument enables text wrapping mode.
 * Returns 0 on success, or a non-zero value on error.
 */
int main(int argc, char **argv)
{
    /* Allow zero or one argument: optional "--wrap" */
    if (argc > 3) {
	usage(argv[0]);
	return 1;
    }

    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "--wrap") == 0) {
	    wrap_mode = 1;
	} else if (strcmp(argv[1], "--debug") == 0) {
	    debug = 1;
	} else {
	    usage(argv[0]);
	    return 1;
	}
    }

    char *rand_file = NULL;
    char *file_contents = NULL;
    char *full_path = NULL;

    seed_rng();

    rand_file = random_file_from_dir();
    if (!rand_file) {
	perror("random_file_from_dir");
	return 1;
    }

    full_path = malloc(MAX_PATH_SIZE);
    if (!full_path) {
	perror("malloc");
	return 1;
    }
    snprintf(full_path, MAX_PATH_SIZE, "%s/%s", ENTRIES_DIR, rand_file);
    if (debug == 1) {
	fprintf(stdout, "[debug] reading %s\n", full_path);
    }

    file_contents = read_file(full_path);
    if (!file_contents) {
	perror("read_file");
	if (full_path != NULL) {
	    free(full_path);
	}
	return 1;
    }

    /* Launch the typing trainer interface */
    run_typing_trainer(full_path, file_contents);

    free(file_contents);
    free(full_path);
    return 0;
}

int mkpath(const char *dir, mode_t mode)
{
    char *tmp = strdup(dir);
    if (tmp == NULL) {
	perror("strdup");
	return -1;
    }

    size_t len = strlen(tmp);
    if (len == 0) {
	free(tmp);
	return 0;
    }
    // Remove trailing slash, if any.
    if (tmp[len - 1] == '/') {
	tmp[len - 1] = '\0';
    }
    // Create each directory component in the path.
    for (char *p = tmp + 1; *p; p++) {
	if (*p == '/') {
	    *p = '\0';
	    if (mkdir(tmp, mode) != 0) {
		if (errno != EEXIST) {
		    perror(tmp);
		    free(tmp);
		    return -1;
		}
	    }
	    *p = '/';
	}
    }

    // Create the final directory.
    if (mkdir(tmp, mode) != 0) {
	if (errno != EEXIST) {
	    perror(tmp);
	    free(tmp);
	    return -1;
	}
    }

    free(tmp);
    return 0;
}

int __create_directories(const char *path)
{
    char *tmp = strdup(path);
    if (tmp == NULL) {
	perror("strdup");
	return -1;
    }

    size_t len = strlen(tmp);
    if (len == 0) {
	free(tmp);
	return 0;
    }
    // Remove trailing slash, if any.
    if (tmp[len - 1] == '/') {
	tmp[len - 1] = '\0';
    }
    // Create each directory component in the path.
    for (char *p = tmp + 1; *p; p++) {
	if (*p == '/') {
	    *p = '\0';
	    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
		perror("mkdir");
		free(tmp);
		return -1;
	    }
	    *p = '/';
	}
    }

    // Create the final directory.
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
	perror("mkdir");
	free(tmp);
	return -1;
    }

    free(tmp);
    return 0;
}

int create_data_csv(void)
{

    home_dir = getenv("HOME");
    if (home_dir == NULL) {
	fprintf(stderr, "Error: HOME environment variable is not set.\n");
	return -1;
    }
    // Construct the full path to the data.csv file.
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", home_dir, scores_file);

    scores_file = path;

    // Extract the directory path by removing the file name.
    char *dir_path = strdup(path);
    if (dir_path == NULL) {
	perror("strdup");
	return -1;
    }
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
	*last_slash = '\0';
    }
    // Create the necessary directories.
    if (__create_directories(dir_path) != 0) {
	free(dir_path);
	return -1;
    }
    free(dir_path);

    // Open the file in append mode (creates it if it doesn't exist).
    FILE *file = fopen(path, "a");
    if (file == NULL) {
	perror("fopen");
	return -1;
    }
    fclose(file);

    if (debug == 1) {
	fprintf(stderr, "[debug] File created or already exists: %s\n",
		path);
    }
    return 0;
}

char *read_file(const char *path)
{
    char *buffer = NULL;
    long length;
    FILE *f = fopen(path, "rb");

    if (!f) {
	perror("fopen");
	return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
	perror("Error seeking EOF");
	fclose(f);
	return NULL;
    }

    length = ftell(f);
    if (length < 0) {
	perror("Error getting file length");
	fclose(f);
	return NULL;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
	perror("Error seeking to start of file");
	fclose(f);
	return NULL;
    }

    /* Allocate extra byte for null terminator */
    buffer = malloc((size_t) length + 1);
    if (!buffer) {
	perror("Error allocating buffer");
	fclose(f);
	return NULL;
    }

    if (fread(buffer, 1, (size_t) length, f) != (size_t) length) {
	perror("Error reading file");
	free(buffer);
	fclose(f);
	return NULL;
    }
    buffer[length] = '\0';

    if (fclose(f) != 0) {
	perror("Error closing file");
	free(buffer);
	return NULL;
    }

    return buffer;
}

void calc_speed(const char *filename, double elapsed, double *wpm,
		double *cpm)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
	perror("Error opening file");
	exit(EXIT_FAILURE);
    }

    int total_chars = 0;
    int ch;

    /* Count only non-whitespace characters */
    while ((ch = fgetc(file)) != EOF) {
	if (!isspace(ch)) {
	    total_chars++;
	}
    }
    fclose(file);

    *cpm = ((double) total_chars / elapsed) * 60.0;
    *wpm = *cpm / 5.0;		/* Assuming average word length is 5 characters */
}

char *random_file_from_dir(void)
{
    size_t file_count = 0;
    char **entries = collect_dir_entries(ENTRIES_DIR, &file_count);
    char *selected = NULL;
    char *entry = NULL;

    if (!entries || file_count == 0)
	return NULL;

    /* Get a random entry from the list */
    entry = get_random_entry(entries, file_count);
    selected = strdup(entry);
    if (!selected)
	perror("strdup");

    /* Free all entries */
    for (size_t i = 0; i < file_count; i++)
	free(entries[i]);
    free(entries);

    return selected;
}

char *get_random_entry(char **files, size_t count)
{
    size_t random_index = (size_t) (rand() % count);
    return files[random_index];
}

char **collect_dir_entries(const char *path, size_t *count)
{
    struct dirent *dent;
    DIR *dir;
    char **file_array = NULL;
    size_t file_count = 0;
    char **temp = NULL;

    dir = opendir(path);
    if (!dir) {
	perror("opendir");
	return NULL;
    }

    while ((dent = readdir(dir)) != NULL) {
	/* Only consider regular files */
	if ((unsigned char) dent->d_type != (unsigned char) REGULAR_FILE)
	    continue;

	temp = realloc(file_array, sizeof(char *) * (file_count + 1));
	if (!temp) {
	    perror("realloc");
	    for (size_t i = 0; i < file_count; i++)
		free(file_array[i]);
	    free(file_array);
	    closedir(dir);
	    return NULL;
	}
	file_array = temp;

	file_array[file_count] = strdup(dent->d_name);
	if (!file_array[file_count]) {
	    perror("strdup");
	    for (size_t i = 0; i < file_count; i++)
		free(file_array[i]);
	    free(file_array);
	    closedir(dir);
	    return NULL;
	}
	file_count++;
    }

    closedir(dir);
    *count = file_count;
    return file_array;
}

void seed_rng(void)
{
    srand((unsigned int) time(NULL));
}

void save_score(double wpm, double cpm, double accuracy,
		double consistency, char *path)
{
    if (create_data_csv() != 0) {
	fprintf(stderr, "Failed to create data csv path %s/%s",
		home_dir, scores_file);
	if (path != NULL) {
	    free(path);
	}
	if (home_dir != NULL) {
	    free(home_dir);
	}
	exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(scores_file, "a");
    if (!fp) {
	perror("fopen scores file");
	return;
    }
    /* Save score in CSV format: WPM,CPM,Accuracy,Consistency,Path */
    fprintf(fp, "%.2f,%.2f,%.2f,%.2f,%s\n", wpm, cpm, accuracy,
	    consistency, path);
    fclose(fp);
}

void draw_scrolled(const char *text, int total_chars, size_t i,
		   int screen_width, char *typed, int current_index)
{
    /* Original horizontal scrolling mode */
    int offset = (current_index + char_offset < screen_width)
	? 0 : current_index - screen_width + char_offset + 1;

    for (i = offset; (int) i < current_index; i++) {
	if ((int) i >= total_chars)
	    break;
	if (typed[i] == text[i]) {
	    attron(COLOR_PAIR(1));
	    mvaddch(0, i - offset, typed[i]);
	    attroff(COLOR_PAIR(1));
	} else {
	    attron(COLOR_PAIR(3));
	    mvaddch(0, i - offset, HIDE_ERR ? text[i] : typed[i]);
	    attroff(COLOR_PAIR(3));
	}
    }
    attron(COLOR_PAIR(2));
    attron(A_DIM);
    mvprintw(0, current_index - offset, "%.*s",
	     screen_width - (current_index - offset),
	     text + current_index);
    attroff(A_DIM);
    attroff(COLOR_PAIR(2));
}

void draw_wrapped(const char *text, int total_chars,
		  int screen_width, char *typed, int current_index)
{
    /* In wrap mode, use fixed width wrapping */
    for (int i = 0; i < total_chars; i++) {
	int row = i / screen_width;
	int col = i % screen_width;
	if (i < current_index) {
	    if (typed[i] == text[i]) {
		attron(COLOR_PAIR(1));
		mvaddch(row, col, typed[i]);
		attroff(COLOR_PAIR(1));
	    } else {
		attron(COLOR_PAIR(3));
		/* If HIDE_ERR is true, show expected char */
		mvaddch(row, col, HIDE_ERR ? text[i] : typed[i]);
		attroff(COLOR_PAIR(3));
	    }
	} else {
	    attron(COLOR_PAIR(2));
	    attron(A_DIM);
	    mvaddch(row, col, text[i]);
	    attroff(A_DIM);
	    attroff(COLOR_PAIR(2));
	}
    }
}

void run_typing_trainer(char *path, const char *text)
{
    size_t total_chars = strlen(text);
    size_t current_index = 0;
    int ch;
    time_t start_time = 0, end_time = 0;
    int started = 0;		/* 0 = not started, 1 = started */
    int screen_width;
    size_t i = 0;
    /* Allocate buffer for user's input */
    char *typed = malloc(total_chars + 1);
    if (!typed)
	return;
    memset(typed, 0, total_chars + 1);

    /* Counters for keystrokes and errors for consistency */
    int total_keystrokes = 0;
    int error_count = 0;

    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);		/* hide cursor */

    if (has_colors()) {
	start_color();
	/* Color pair 1: white on black for correct typed text */
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	/* Color pair 2: white on black for untyped text with dim attribute */
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	/* Color pair 3: red on black for incorrect typed text */
	init_pair(3, COLOR_RED, COLOR_BLACK);
	/* Color pair 4: green on black for good accuracy */
	init_pair(4, COLOR_GREEN, COLOR_BLACK);
    }

    /* Typing loop */
    while (current_index < total_chars) {
	clear();
	screen_width = getmaxx(stdscr);

	if (wrap_mode) {
	    draw_wrapped(text, total_chars, screen_width, typed,
			 current_index);
	} else {
	    draw_scrolled(text, total_chars, i, screen_width, typed,
			  current_index);
	}

	refresh();
	ch = getch();
	if (!started) {
	    start_time = time(NULL);
	    started = 1;
	}
	if (ch == KEY_BACKSPACE || ch == PRINT_CHAR_MAX + 1 || ch == 8) {
	    if (current_index > 0)
		current_index--;
	} else if (ch >= PRINT_CHAR_MIN && ch <= PRINT_CHAR_MAX) {	/* Printable characters */
	    total_keystrokes++;
	    typed[current_index] = (char) ch;
	    if ((unsigned char) ch != (unsigned char) text[current_index])
		error_count++;
	    current_index++;
	}
    }
    end_time = time(NULL);

    double elapsed = difftime(end_time, start_time);
    if (elapsed <= 0)
	elapsed = 1;		/* avoid division by zero */
    double wpm, cpm;
    calc_speed(path, elapsed, &wpm, &cpm);

    size_t correct_chars = 0;
    for (i = 0; i < total_chars; i++) {
	if (typed[i] == text[i])
	    correct_chars++;
    }
    double accuracy = ((double) correct_chars * 100.0) / total_chars;
    double consistency = total_keystrokes > 0 ?
	((double) (total_keystrokes - error_count) * 100.0 /
	 total_keystrokes) : 100.0;

    clear();
    mvprintw(0, 0, "WPM: %.6f          CPM: %.2f", wpm, cpm);
    int acc_color_index = 4;	/* green by default */
    if (accuracy < 90.0)
	acc_color_index = 3;
    mvprintw(1, 0, "Accuracy: %.4f%%   Consistency: %.2f%%", accuracy,
	     consistency);
    attroff(COLOR_PAIR(acc_color_index));
    mvprintw(4, 5, "[[ Press any key ]]");
    refresh();
    getch();
    endwin();

    save_score(wpm, cpm, accuracy, consistency, path);
    if (free != NULL) {
	free(typed);
    }
}

void usage(char *progname)
{
    if (progname != NULL) {
	fprintf(stderr, "Usage: %s [--wrap] [--debug]\n", progname);
    }
    exit(EXIT_FAILURE);
}
