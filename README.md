# Typc

This is a program called Typc.

## About the program.

This program is a console-based typing trainer. It's fairly minimal (at least
at the time of this being written) so it doesn't have any fancy command line
parsing features or anything like that. However it does have a few features.

## The features of the program.

The first feature is the ability to calculate the statistics of the text you've
typed. So the program selects a text at random from the texts/ directory. You
go ahead and type the text, and once you've finished, it shows you a few
details about how well you performed in the test. I'll just be calling this the
'results' page.

### Results Page

#### Words per minute

The words per minute is simply the amount of words that you would have typed
per every 60 seconds (1 minute).

#### Characters per minute

Similar to your words per minute, except it is the amount of characters that
you would have typed per minute. Very self-explanatory.

#### Accuracy

Your accuracy is calculated as the amount of characters you have typed
correctly multiplied by 100. This is then divided by the total amount of
characters in the text.

$$
accuracy = \frac{correct\text{ }characters \cdot 100}{total\text{ }characters}
$$

#### Consistency

The consistency is the amount of mistakes that you've made, even the ones that
you've corrected, proportional to the total amount of keystrokes.

$$
consistency = \frac{(keystrokes - errors) \cdot 100}{keystrokes}
$$

## Why this program was made.

This program was made because there was a significant lack of command-line
typing trainers that were/are open source. There is a program that is build
with Rust called typrr (or something similar) which I found to be a decent
command line typing trainer, but it required Rust to be installed. The reason
why this is a problem for me is a bit complex, but it boils down to the idea
that unless there is a very good reason for not doing so, programs should be
built in C. Programs that are built in C can be cross-compiled (with
some exceptions) and are generally very fast as well. For these reasons, Typc
is build in C.

## The name of the program.

The program's name was derived from the name of an existing Rust program for
typing called Typerr and I took the name, changed it a bit, wacked a C on the
end, and here we are.

## Using the program.

Using the program is very simple. You simply just run the executable (within
the source directory). It's important that the progrma is run within the source
directory because the program uses something called relative IO pathing. This
basically means that various specific directories and files must be present in the
same directory where the program is run. For instance, the 'texts' directory
must be visible, as that is where all of the texts are stored. Running the
program in a ~/Downloads/typc folder where ~/Downloads/typc/texts doesn't exist
would be problematic for this exact reason.

## How the program chooses a text to show to you for you to type.

The program gets all the files in the 'texts' directory, and selects one at
random. The RNG is seeded from the current time so that texts are selected in a
different random sequence every time the program is run. If you run the
program, type a few randomly selected texts and then exit, the next time you
run the program, the sequence in which the texts are selected is in a
completely different random order.

## Adding new texts.

You can add new texts very easily. You can create a new file in 'texts'. You
can also call it whatever you'd like, but typically you should name it
something that allows you to easily know what the text is about. If the text
was a quote from the Bible you might name it something like
"bible_verse_corinthians_12" or something like that. Of course, that's not
required, and you can really name it whatever you want, but the texts that are
provided by default have file names that somewhat match the topic.
