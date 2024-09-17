// School project

#include <stdio.h>
#include <stdlib.h>

int main() {
    char answer;
    
    printf("Does the animal have wings? [y/n]: ");
    if (scanf(" %c", &answer) != 1) {
        printf("Invalid input\n");
        return 1;
    }

    if (answer == 'y') {
        printf("The animal is a bird.\n");
    } else {
        printf("Does the animal catch mice? [y/n]: ");
        if (scanf(" %c", &answer) != 1) {
            printf("Invalid input\n");
            return 1;
        }
        printf("It's a %s\n", answer == 'y' ? "cat" : "dog");
    }

    return 0;
}
