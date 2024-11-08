#include <stdio.h>
#include <stdlib.h>

#define MAX_NUMBERS 100

int main() {
    double numbers[MAX_NUMBERS];  // Array to store the numbers
    int count = 0;               // Counter for number of entries
    double sum = 0.0;           // Sum of all numbers
    char choice;                // User's choice to continue or stop

    printf("Enter numbers to calculate their average.\n");
    printf("After each number, press 'y' to continue or 'n' to finish.\n\n");

    // Collect numbers from user
    do {
        if (count >= MAX_NUMBERS) {
            printf("Maximum limit of %d numbers reached.\n", MAX_NUMBERS);
            break;
        }

        printf("Enter number %d: ", count + 1);
        
        // Input validation
        while (scanf("%lf", &numbers[count]) != 1) {
            printf("Invalid input. Please enter a number: ");
            while (getchar() != '\n');  // Clear input buffer
        }

        // Add to sum
        sum += numbers[count];
        count++;

        // Ask if user wants to continue
        printf("Do you want to enter another number? (y/n): ");
        while (getchar() != '\n');  // Clear input buffer
        choice = getchar();
        while (getchar() != '\n');  // Clear input buffer again

    } while (choice == 'y' || choice == 'Y');

    // Calculate and display average
    if (count > 0) {
        double average = sum / count;
        
        printf("\nNumbers entered: ");
        for (int i = 0; i < count; i++) {
            printf("%.2f", numbers[i]);
            if (i < count - 1) {
                printf(", ");
            }
        }
        
        printf("\nCount: %d\n", count);
        printf("Sum: %.2f\n", sum);
        printf("Average: %.2f\n", average);
    } else {
        printf("\nNo numbers were entered.\n");
    }

    return 0;
}