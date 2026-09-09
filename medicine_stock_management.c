#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_MEDICINES 100

// Enums for stock condition (lower value = higher severity)
enum StockCondition {
    CRITICAL_CONDITION = 1,
    COMBINED_SHORTAGE_EXPIRY = 2,
    URGENT_REORDER = 3,
    EXPIRY_ATTENTION = 4,
    REORDER_REQUIRED = 5,
    SUFFICIENT_STOCK = 6
};

// Struct definition
struct Medicine {
    char code[10];
    char name[50];
    int availableStock;
    int avgDailyReq;
    int minStockLevel;
    int daysToExpiry;
    int essentiality;
    
    double stockCoverage;
    enum StockCondition stockCondition;
    int priorityScore;
};

// Function prototypes
void initializeData(struct Medicine meds[], int *count);
void calculateDerivedFields(struct Medicine *med);
void processAllMedicines(struct Medicine meds[], int count);
void displayMenu();
int searchMedicine(struct Medicine meds[], int count, const char* code);
void displaySingleMedicine(struct Medicine med);
void updateStock(struct Medicine meds[], int count, int isIssue);
void sortMedicines(struct Medicine meds[], int count);
void generateReport(struct Medicine meds[], int count, int toFile);
void displaySummary(struct Medicine meds[], int count, int toFile);
const char* getConditionString(enum StockCondition cond);
void clearInputBuffer();

int main() {
    struct Medicine meds[MAX_MEDICINES];
    int count = 0;
    int choice = 0;
    
    initializeData(meds, &count);
    processAllMedicines(meds, count);
    sortMedicines(meds, count);
    
    while (1) {
        displayMenu();
        if (scanf("%d", &choice) != 1) {
            if (feof(stdin)) {
                break;
            }
            clearInputBuffer();
            printf("\nInvalid input. Please enter a number.\n");
            continue;
        }
        clearInputBuffer();
        
        switch (choice) {
            case 1: { // Search
                char searchCode[20];
                printf("\nEnter Medicine Code to search: ");
                if (fgets(searchCode, sizeof(searchCode), stdin)) {
                    searchCode[strcspn(searchCode, "\n")] = '\0';
                    int index = searchMedicine(meds, count, searchCode);
                    if (index != -1) {
                        displaySingleMedicine(meds[index]);
                    } else {
                        printf("\nError: Medicine code '%s' not found in the system.\n", searchCode);
                    }
                }
                break;
            }
            case 2: // Receive
                updateStock(meds, count, 0);
                sortMedicines(meds, count);
                break;
            case 3: // Issue
                updateStock(meds, count, 1);
                sortMedicines(meds, count);
                break;
            case 4: // View Report
                generateReport(meds, count, 0);
                displaySummary(meds, count, 0);
                break;
            case 5: // Save Report
                generateReport(meds, count, 1);
                displaySummary(meds, count, 1);
                printf("\nReport successfully saved to stock_report.txt\n");
                break;
            case 6: // Exit
                printf("\nExiting program...\n");
                return 0;
            default:
                printf("\nInvalid choice. Please try again.\n");
        }
    }
    
    return 0;
}

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void initializeData(struct Medicine meds[], int *count) {
    // Initial 6 dataset entries
    struct Medicine initial[] = {
        {"MED01", "Oral Saline", 120, 35, 80, 45, 3, 0.0, SUFFICIENT_STOCK, 0},
        {"MED02", "Paracetamol", 300, 40, 100, 120, 2, 0.0, SUFFICIENT_STOCK, 0},
        {"MED03", "Insulin",     60,  12, 50,  30,  3, 0.0, SUFFICIENT_STOCK, 0},
        {"MED04", "Amoxicillin", 200, 25, 80,  20,  3, 0.0, SUFFICIENT_STOCK, 0},
        {"MED05", "Antacid",     250, 18, 60,  15,  1, 0.0, SUFFICIENT_STOCK, 0},
        {"MED06", "Cetirizine",  180, 15, 50,  90,  1, 0.0, SUFFICIENT_STOCK, 0}
    };
    
    *count = 6;
    for (int i = 0; i < *count; i++) {
        meds[i] = initial[i];
    }
}

void calculateDerivedFields(struct Medicine *med) {
    // 1. Calculate Stock Coverage
    if (med->avgDailyReq > 0) {
        med->stockCoverage = (double)med->availableStock / med->avgDailyReq;
    } else {
        med->stockCoverage = 9999.0;
    }
    
    // 2. Calculate Stock Condition (Top-down evaluation)
    if (med->essentiality == 3 && med->stockCoverage < 7.0) {
        med->stockCondition = CRITICAL_CONDITION;
    } 
    else if (med->availableStock < med->minStockLevel && 
            (med->daysToExpiry <= 30 || med->stockCoverage > med->daysToExpiry)) {
        med->stockCondition = COMBINED_SHORTAGE_EXPIRY;
    }
    else if (med->availableStock < med->minStockLevel && med->essentiality >= 2) {
        med->stockCondition = URGENT_REORDER;
    }
    else if (med->daysToExpiry <= 30 || med->stockCoverage > med->daysToExpiry) {
        med->stockCondition = EXPIRY_ATTENTION;
    }
    else if (med->availableStock < med->minStockLevel) {
        med->stockCondition = REORDER_REQUIRED;
    }
    else {
        med->stockCondition = SUFFICIENT_STOCK;
    }
    
    // 3. Calculate Management Priority Score
    int score = 0;
    
    // Essentiality
    score += med->essentiality * 20;
    
    // Shortage
    if (med->availableStock < med->minStockLevel) {
        score += 30;
    }
    
    // Coverage
    if (med->stockCoverage < 7.0) {
        score += 20;
    } else if (med->stockCoverage < 14.0) {
        score += 10;
    }
    
    // Expiry
    if (med->daysToExpiry <= 15) {
        score += 20;
    } else if (med->daysToExpiry <= 30) {
        score += 10;
    }
    
    // Wastage
    if (med->stockCoverage > med->daysToExpiry) {
        score += 15;
    }
    
    med->priorityScore = score;
}

void processAllMedicines(struct Medicine meds[], int count) {
    for (int i = 0; i < count; i++) {
        calculateDerivedFields(&meds[i]);
    }
}

void displayMenu() {
    printf("\n=========================================\n");
    printf("   Medicine Stock Management System\n");
    printf("=========================================\n");
    printf("1. Search Medicine\n");
    printf("2. Receive Stock (Add)\n");
    printf("3. Issue Medicine (Subtract)\n");
    printf("4. View Final Stock Report\n");
    printf("5. Save Final Report to File\n");
    printf("6. Exit\n");
    printf("-----------------------------------------\n");
    printf("Enter choice: ");
}

int searchMedicine(struct Medicine meds[], int count, const char* code) {
    for (int i = 0; i < count; i++) {
        // Case insensitive compare (simple implementation)
        const char *p1 = meds[i].code;
        const char *p2 = code;
        int match = 1;
        while (*p1 != '\0' || *p2 != '\0') {
            if (tolower((unsigned char)*p1) != tolower((unsigned char)*p2)) {
                match = 0;
                break;
            }
            p1++;
            p2++;
        }
        if (match) {
            return i;
        }
    }
    return -1;
}

const char* getConditionString(enum StockCondition cond) {
    switch (cond) {
        case CRITICAL_CONDITION: return "Critical Condition";
        case COMBINED_SHORTAGE_EXPIRY: return "Combined Shortage & Expiry";
        case URGENT_REORDER: return "Urgent Reorder";
        case EXPIRY_ATTENTION: return "Expiry Attention";
        case REORDER_REQUIRED: return "Reorder Required";
        case SUFFICIENT_STOCK: return "Sufficient Stock";
        default: return "Unknown";
    }
}

void displaySingleMedicine(struct Medicine med) {
    printf("\n--- Medicine Details ---\n");
    printf("Code:                  %s\n", med.code);
    printf("Name:                  %s\n", med.name);
    printf("Available Stock:       %d\n", med.availableStock);
    printf("Avg Daily Req:         %d\n", med.avgDailyReq);
    printf("Minimum Stock Level:   %d\n", med.minStockLevel);
    printf("Days to Expiry:        %d\n", med.daysToExpiry);
    printf("Essentiality Level:    %d\n", med.essentiality);
    printf("Estimated Coverage:    %.2f days\n", med.stockCoverage);
    printf("Stock Condition:       %s\n", getConditionString(med.stockCondition));
    printf("Priority Score:        %d\n", med.priorityScore);
    printf("------------------------\n");
}

void updateStock(struct Medicine meds[], int count, int isIssue) {
    char searchCode[20];
    int qty;
    
    printf("\nEnter Medicine Code: ");
    if (fgets(searchCode, sizeof(searchCode), stdin)) {
        searchCode[strcspn(searchCode, "\n")] = '\0';
    } else {
        return;
    }
    
    int index = searchMedicine(meds, count, searchCode);
    if (index == -1) {
        printf("\nError: Medicine code '%s' not found.\n", searchCode);
        return;
    }
    
    if (isIssue) {
        printf("Enter quantity to issue: ");
    } else {
        printf("Enter quantity to receive: ");
    }
    
    if (scanf("%d", &qty) != 1) {
        clearInputBuffer();
        printf("\nError: Invalid quantity input.\n");
        return;
    }
    clearInputBuffer();
    
    if (qty <= 0) {
        printf("\nError: Quantity must be greater than zero.\n");
        return;
    }
    
    if (isIssue) {
        if (qty > meds[index].availableStock) {
            printf("\nError: Issued quantity (%d) exceeds available stock (%d).\n", qty, meds[index].availableStock);
            return;
        }
        meds[index].availableStock -= qty;
        printf("\nSuccessfully issued %d units of %s.\n", qty, meds[index].code);
    } else {
        meds[index].availableStock += qty;
        printf("\nSuccessfully received %d units of %s.\n", qty, meds[index].code);
    }
    
    // Recalculate derived fields
    calculateDerivedFields(&meds[index]);
}

void sortMedicines(struct Medicine meds[], int count) {
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            int swap = 0;
            
            // 1. Higher Priority Score wins
            if (meds[j].priorityScore < meds[j+1].priorityScore) {
                swap = 1;
            } else if (meds[j].priorityScore == meds[j+1].priorityScore) {
                // Tie-breakers
                // 1st: Higher Essentiality Level
                if (meds[j].essentiality < meds[j+1].essentiality) {
                    swap = 1;
                } else if (meds[j].essentiality == meds[j+1].essentiality) {
                    // 2nd: Lower Stock Coverage
                    if (meds[j].stockCoverage > meds[j+1].stockCoverage) {
                        swap = 1;
                    } else if (meds[j].stockCoverage == meds[j+1].stockCoverage) {
                        // 3rd: Lower Days Remaining to Expiry
                        if (meds[j].daysToExpiry > meds[j+1].daysToExpiry) {
                            swap = 1;
                        } else if (meds[j].daysToExpiry == meds[j+1].daysToExpiry) {
                            // 4th: Alphabetically smaller Medicine Code
                            if (strcmp(meds[j].code, meds[j+1].code) > 0) {
                                swap = 1;
                            }
                        }
                    }
                }
            }
            
            if (swap) {
                struct Medicine temp = meds[j];
                meds[j] = meds[j+1];
                meds[j+1] = temp;
            }
        }
    }
}

void generateReport(struct Medicine meds[], int count, int toFile) {
    FILE *fp = NULL;
    if (toFile) {
        fp = fopen("stock_report.txt", "w");
        if (fp == NULL) {
            printf("\nError: Could not open file for writing.\n");
            return;
        }
    }
    
    #define OUT(...) do { \
        if (toFile) fprintf(fp, __VA_ARGS__); \
        else printf(__VA_ARGS__); \
    } while(0)
    
    OUT("\n=========================================================================================================================\n");
    OUT("%-8s | %-15s | %-6s | %-8s | %-10s | %-5s | %-28s | %-8s\n", 
        "Code", "Name", "Stock", "Cov(d)", "Exp(d)", "Ess", "Condition", "Priority");
    OUT("=========================================================================================================================\n");
    
    for (int i = 0; i < count; i++) {
        OUT("%-8s | %-15s | %-6d | %-8.2f | %-10d | %-5d | %-28s | %-8d\n",
            meds[i].code,
            meds[i].name,
            meds[i].availableStock,
            meds[i].stockCoverage,
            meds[i].daysToExpiry,
            meds[i].essentiality,
            getConditionString(meds[i].stockCondition),
            meds[i].priorityScore);
    }
    OUT("=========================================================================================================================\n");
    
    if (toFile) {
        fclose(fp);
    }
}

void displaySummary(struct Medicine meds[], int count, int toFile) {
    FILE *fp = NULL;
    if (toFile) {
        fp = fopen("stock_report.txt", "a");
        if (fp == NULL) return;
    }
    
    int reorderCount = 0;
    int urgentCount = 0;
    int expiryConcernCount = 0;
    
    for (int i = 0; i < count; i++) {
        if (meds[i].stockCondition == REORDER_REQUIRED || 
            meds[i].stockCondition == URGENT_REORDER || 
            meds[i].stockCondition == COMBINED_SHORTAGE_EXPIRY || 
            meds[i].stockCondition == CRITICAL_CONDITION) {
            reorderCount++;
        }
        
        if (meds[i].stockCondition == URGENT_REORDER || 
            meds[i].stockCondition == CRITICAL_CONDITION) {
            urgentCount++;
        }
        
        if (meds[i].stockCondition == EXPIRY_ATTENTION || 
            meds[i].stockCondition == COMBINED_SHORTAGE_EXPIRY) {
            expiryConcernCount++;
        }
    }
    
    #define OUTS(...) do { \
        if (toFile) fprintf(fp, __VA_ARGS__); \
        else printf(__VA_ARGS__); \
    } while(0)
    
    OUTS("\n--- Summary ---\n");
    OUTS("Total medicines analysed:              %d\n", count);
    OUTS("Number requiring reorder:              %d\n", reorderCount);
    OUTS("Number requiring urgent attention:     %d\n", urgentCount);
    OUTS("Number having expiry concern:          %d\n", expiryConcernCount);
    
    if (count > 0) {
        OUTS("Medicine requiring highest priority:   %s (%s)\n", meds[0].code, meds[0].name);
    }
    OUTS("---------------\n");
    
    if (toFile) {
        fclose(fp);
    }
}
