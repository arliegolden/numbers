// School project

#include <stdio.h>

double width, height, woodLength, glassArea;

int main() {
    printf("Enter the width: ");
    if (scanf("%lf", &width) != 1) {
        printf("Invalid input\n");
        return 1;
    }

    printf("Enter the height: ");
    if (scanf("%lf", &height) != 1) {
        printf("Invalid input\n");
        return 1;
    }

    woodLength = 2 * (width + height) * 3.25;
    glassArea = 2 * (width + height);

    printf("The wood required is %.2lf feet.\n", woodLength);
    printf("The glass required is %.2lf square meters.\n", glassArea);
}