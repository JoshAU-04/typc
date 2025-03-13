#include <dirent.h>
#include <limits.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define REGULAR_FILE   DT_REG
#define MAX_PATH_SIZE  (PATH_MAX)
#define ENTRIES_DIR    "./texts"
#define CHAR_OFFSET    20

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
 * run_typing_trainer - Run an ncurses-based typing trainer.
 * @text: The text to be typed by the user.
 *
 * Uses ncurses to display the text for typing. The text already typed is shown
 * in white, while the text yet to be typed is shown in dim (grayish) style.
 * Implements horizontal scrolling so that the current character being typed is
 * always visible even when the text exceeds the screen width.
 * Once the user finishes typing, calculates and displays the words-per-minute (WPM) score.
 */
static void run_typing_trainer(const char *text);

/**
 * main - Entry point for the typing trainer program.
 *
 * Reads a random text file from the ENTRIES_DIR directory and launches the typing trainer.
 * Returns 0 on success, or a non-zero value on error.
 */
int main(void)
{
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
	free(rand_file);
	return 1;
    }
    snprintf(full_path, MAX_PATH_SIZE, "%s/%s", ENTRIES_DIR, rand_file);
    fprintf(stdout, "[debug] reading %s\n", full_path);

    file_contents = read_file(full_path);
    if (!file_contents) {
	perror("read_file");
	free(full_path);
	free(rand_file);
	return 1;
    }

    /* Launch the typing trainer interface */
    run_typing_trainer(file_contents);

    free(file_contents);
    free(full_path);
    free(rand_file);
    return 0;
}

static char *read_file(const char *path)
{
    char *buffer = NULL;
    long length;
    FILE *f = fopen(path, "rb");

    if (!f) {
	perror("fopen");
	return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
	perror("fseek");
	fclose(f);
	return NULL;
    }

    length = ftell(f);
    if (length < 0) {
	perror("ftell");
	fclose(f);
	return NULL;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
	perror("fseek");
	fclose(f);
	return NULL;
    }

    /* Allocate extra byte for null terminator */
    buffer = malloc((size_t) length + 1);
    if (!buffer) {
	perror("malloc");
	fclose(f);
	return NULL;
    }

    if (fread(buffer, 1, (size_t) length, f) != (size_t) length) {
	perror("fread");
	free(buffer);
	fclose(f);
	return NULL;
    }
    buffer[length] = '\0';

    if (fclose(f) != 0) {
	perror("fclose");
	free(buffer);
	return NULL;
    }

    return buffer;
}

static char *random_file_from_dir(void)
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

static char *get_random_entry(char **files, size_t count)
{
    size_t random_index = (size_t) (rand() % count);
    return files[random_index];
}

static char **collect_dir_entries(const char *path, size_t *count)
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

static void seed_rng(void)
{
    srand((unsigned int) time(NULL));
}

static void run_typing_trainer(const char *text)
{
    size_t total_chars = strlen(text);
    size_t current_index = 0;
    int ch;
    time_t start_time = 0, end_time = 0;
    int started = 0;		/* flag: 0 = not started, 1 = started */
    int screen_width, offset;

    /* Initialize ncurses */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);		/* hide cursor */

    /* Initialize colors if available */
    if (has_colors()) {
	start_color();
	/* Color pair 1: white on black for typed text */
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	/* Color pair 2: white on black for untyped text with dim attribute */
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
    }

    /* Typing loop with horizontal scrolling */
    while (current_index < total_chars) {
	clear();
	screen_width = getmaxx(stdscr);
	/* Calculate offset so the current character is always visible */
	offset =
	    (current_index + CHAR_OFFSET <
	     (size_t) screen_width) ? 0 : current_index - screen_width +
	    CHAR_OFFSET;

	/* Draw the text already typed in white (normal) */
	attron(COLOR_PAIR(1));
	mvprintw(0, 0, "%.*s", (int) (current_index - offset),
		 text + offset);
	attroff(COLOR_PAIR(1));

	/* Draw the text yet to be typed in dim (grayish) */
	attron(COLOR_PAIR(2));
	attron(A_DIM);
	mvprintw(0, current_index - offset, "%.*s",
		 screen_width - (current_index - offset),
		 text + current_index);
	attroff(A_DIM);
	attroff(COLOR_PAIR(2));

	refresh();

	ch = getch();
	if (!started) {
	    start_time = time(NULL);
	    started = 1;
	}
	if ((unsigned char) ch == (unsigned char) text[current_index]) {
	    current_index++;
	} else {
	    beep();
	}
    }
    end_time = time(NULL);

    /* Calculate WPM: assume 5 characters per word */
    {
	double elapsed = difftime(end_time, start_time);
	if (elapsed <= 0)
	    elapsed = 1;	/* avoid division by zero */
	double wpm = ((double) total_chars / 5.0) * (60.0 / elapsed);
	clear();
	mvprintw(0, 0, "Finished! WPM: %.2f", wpm);
	mvprintw(2, 0, "Press any key to exit...");
	refresh();
	getch();
    }
    endwin();
}
