#include <dirent.h>
#include <limits.h>
#include <ncurses.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define REGULAR_FILE   DT_REG
#define MAX_PATH_SIZE  (PATH_MAX)
#define ENTRIES_DIR    "./texts"
#define CHAR_OFFSET    20
#define SCORES_FILE    "./data/scores.csv"

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
 * @elapsed: Total elapsed time. Assumed to be 0.0.
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
 * @path: Name of the file path
 *
 * Opens the scores file (creating it if necessary) and appends a new line containing
 * the WPM, CPM, accuracy, and consistency metrics, separated by commas.
 */
static void save_score(double wpm, double cpm, double accuracy,
		       double consistency, char* path);


/**
 * run_typing_trainer - Run an ncurses-based typing trainer.
 * @text: The text to be typed by the user.
 * @path: Path to the file containing the text.
 *
 * Uses ncurses to display the text for typing.
 * The already typed text is displayed character-by-character:
 * - Correct characters are shown in white.
 * - Incorrect characters are shown in red.
 * The untyped text is rendered in a dim (grayish) style.
 * Horizontal scrolling is implemented so that the current position remains visible.
 * Backspace support allows corrections.
 * Upon completion, the function calculates and displays both the words-per-minute (WPM)
 * characters-per-minute (CPM), and the accuracy metric.
 */
static void run_typing_trainer(char *path, const char *text);

/**
 *usage - Print program usage.
 * @progname: The program name
 * 
 * Send the program usage details into stderr pipe.
 */
static void usage(char* progname);

/**
 * main - Entry point for the typing trainer program.
 *
 * Reads a random text file from the ENTRIES_DIR directory and launches the typing trainer.
 * Returns 0 on success, or a non-zero value on error.
 */
int main(int argc, char** argv)
{
    if (argc == 1) {
	usage(argv[0]);
	return 1;
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
    run_typing_trainer(full_path, file_contents);

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

static void calc_speed(const char *filename, double elapsed, double *wpm,
		       double *cpm)
{
    FILE *file = fopen(filename, "r");
    if (!file) {
	perror("Error opening file");
	exit(EXIT_FAILURE);
    }

    int total_chars = 0;
    int ch;

    // Read the file character by character and count only non-space characters
    while ((ch = fgetc(file)) != EOF) {
	if (!isspace(ch)) {
	    total_chars++;
	}
    }
    fclose(file);

    // Calculate characters per minute and words per minute
    *cpm = ((double) total_chars / elapsed) * 60.0;
    *wpm = *cpm / 5.0;		// Assuming average word length is 5 characters
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

static void save_score(double wpm, double cpm, double accuracy,
		       double consistency, char* path)
{
    FILE *fp = fopen(SCORES_FILE, "a");
    if (!fp) {
	perror("fopen scores file");
	return;
    }
    /* Save score in CSV format: WPM,CPM,Accuracy,Consistency,Path */
    fprintf(fp, "%.2f,%.2f,%.2f,%.2f,%s\n", wpm, cpm, accuracy, consistency, path);
    fclose(fp);
}

static void run_typing_trainer(char* path, const char *text)
{
    size_t total_chars = strlen(text);
    size_t current_index = 0;
    int ch;
    time_t start_time = 0, end_time = 0;
    int started = 0;		/* flag: 0 = not started, 1 = started */
    int screen_width, offset;
    size_t i;
    /* Allocate a buffer for the user's input */
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

    /* Initialize colors if available */
    if (has_colors()) {
	start_color();
	/* Color pair 1: white on black for correct typed text */
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	/* Color pair 2: white on black for untyped text with dim attribute */
	init_pair(2, COLOR_WHITE, COLOR_BLACK);
	/* Color pair 3: red on black for incorrect typed text */
	init_pair(3, COLOR_RED, COLOR_BLACK);
    }

    /* Typing loop with horizontal scrolling and backspace support */
    while (current_index < total_chars) {
	clear();
	screen_width = getmaxx(stdscr);
	/* Ensure the current position is visible */
	offset =
	    (current_index + CHAR_OFFSET <
	     (size_t) screen_width) ? 0 : current_index - screen_width +
	    CHAR_OFFSET + 1;

	/* Display the already typed text */
	for (i = offset; i < current_index; i++) {
	    if (i >= total_chars)
		break;
	    if (typed[i] == text[i]) {
		attron(COLOR_PAIR(1));
		mvaddch(0, i - offset, typed[i]);
		attroff(COLOR_PAIR(1));
	    } else {
		attron(COLOR_PAIR(3));
		mvaddch(0, i - offset, text[i]);
		attroff(COLOR_PAIR(3));
	    }
	}

	/* Display the remaining untyped text in dim white */
	(void)attron(COLOR_PAIR(2));
	(void)attron(A_DIM);
	mvprintw(0, current_index - offset, "%.*s",
		 screen_width - (current_index - offset),
		 text + current_index);
	(void)attroff(A_DIM);
	(void)attroff(COLOR_PAIR(2));

	(void)refresh();

	ch = getch();
	if (!started) {
	    start_time = time(NULL);
	    started = 1;
	}
	if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
	    if (current_index > 0)
		current_index--;
	} else if (ch >= 32 && ch <= 126) {	/* Printable characters */
	    total_keystrokes++;
	    typed[current_index] = (char) ch;
	    if ((unsigned char) ch != (unsigned char) text[current_index])
		error_count++;
	    current_index++;
	}
    }
    end_time = time(NULL);

    /* Calculate elapsed time (in seconds), WPM, CPM, and accuracy */
    double elapsed = difftime(end_time, start_time);
    if (elapsed <= 0)
	elapsed = 1;		/* avoid division by zero */
    double wpm, cpm;
    calc_speed(path, elapsed, &wpm, &cpm);


    /* Accuracy based on final text */
    size_t correct_chars = 0;
    for (i = 0; i < total_chars; i++) {
	if (typed[i] == text[i])
	    correct_chars++;
    }
    double accuracy = ((double) correct_chars * 100.0) / total_chars;
    /* Consistency: penalize mistakes even if corrected */
    double consistency = total_keystrokes > 0 ?
	((double) (total_keystrokes - error_count) * 100.0 /
	 total_keystrokes) : 100.0;

    clear();
    mvprintw(0, 0, "Finished! WPM: %.2f   CPM: %.2f", wpm, cpm);
    mvprintw(1, 0, "Accuracy: %.2f%%   Consistency: %.2f%%", accuracy,
	     consistency);
    mvprintw(3, 0, "Press any key to exit...");
    refresh();
    getch();
    endwin();

    /* Save the score to file */
    save_score(wpm, cpm, accuracy, consistency, path);

    free(typed);
}

void usage(char* progname) {
	fprintf(stderr, "Usage: %s [args] <text>\n", progname);
	exit(EXIT_FAILURE);
}
