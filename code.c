#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "inventory.dat"
#define CSV_FILE  "inventory.csv"

struct product {
    char name[50];
    int  id;
    char category[30];
    int  quantity;
    char supplier[50];
    float price;
};

/* ---------- helpers ---------- */

static void flush_stdin_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

static int read_int(const char *prompt, int *out) {
    int n;
    for (;;) {
        printf("%s", prompt);
        n = scanf("%d", out);
        if (n == 1) { flush_stdin_line(); return 1; }
        printf("Invalid number. Try again.\n");
        flush_stdin_line();
    }
}

static int read_float(const char *prompt, float *out) {
    int n;
    for (;;) {
        printf("%s", prompt);
        n = scanf("%f", out);
        if (n == 1) { flush_stdin_line(); return 1; }
        printf("Invalid price. Try again.\n");
        flush_stdin_line();
    }
}

/* read a full line (bounded), trims trailing newline */
static void read_line(const char *prompt, char *buf, size_t cap) {
    printf("%s", prompt);
    if (fgets(buf, (int)cap, stdin) == NULL) {
        /* EOF -> make empty string */
        if (cap) buf[0] = '\0';
        return;
    }
    size_t len = strlen(buf);
    if (len && buf[len - 1] == '\n') buf[len - 1] = '\0';
}

/* escape text for CSV: wrap in quotes and double internal quotes */
static void csv_write_text(FILE *csv, const char *s) {
    fputc('\"', csv);
    const char *p;
    for (p = s; *p; ++p) {
        if (p == '\"') fputc('\"', csv); / double it */
        fputc(*p, csv);
    }
    fputc('\"', csv);
}

/* ---------- features ---------- */

void addproduct(void) {
    struct product p;
    FILE *fp = fopen(DATA_FILE, "ab");
    if (!fp) {
        perror("Error opening data file");
        return;
    }

    read_line("Enter product name: ", p.name, sizeof(p.name));
    read_int("Enter product id: ", &p.id);
    read_line("Enter product category: ", p.category, sizeof(p.category));
    read_int("Enter Quantity: ", &p.quantity);
    read_float("Enter Price: ", &p.price);
    read_line("Enter Supplier Name: ", p.supplier, sizeof(p.supplier));

    size_t wrote = fwrite(&p, sizeof(struct product), 1, fp);
    fclose(fp);

    if (wrote != 1) {
        printf("Write failed! Record not saved.\n");
        return;
    }
    printf("\nProduct added successfully.\n");
}

void viewproduct(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) {
        printf("No inventory data found.\n");
        return;
    }

    struct product p;
    printf("\n%-5s %-20s %-15s %-10s %-10s %-20s\n",
           "ID", "Name", "Category", "Quantity", "Price", "Supplier");
    printf("-------------------------------------------------------------------------------\n");

    while (fread(&p, sizeof(struct product), 1, fp) == 1) {
        printf("%-5d %-20.20s %-15.15s %-10d %-10.2f %-20.20s\n",
               p.id, p.name, p.category, p.quantity, p.price, p.supplier);
    }
    fclose(fp);
}

void searchproduct(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) {
        printf("No inventory data found.\n");
        return;
    }
    int id, found = 0;
    read_int("Enter product ID to search: ", &id);

    struct product p;
    while (fread(&p, sizeof(struct product), 1, fp) == 1) {
        if (p.id == id) {
            printf("\nProduct Found:\n");
            printf("ID: %d\nName: %s\nCategory: %s\nQuantity: %d\nPrice: %.2f\nSupplier: %s\n",
                   p.id, p.name, p.category, p.quantity, p.price, p.supplier);
            found = 1;
            break;
        }
    }
    if (!found) printf("Product with ID %d not found.\n", id);
    fclose(fp);
}

void editproduct(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) { printf("No inventory data found.\n"); return; }

    int id, found = 0;
    read_int("Enter product ID to edit: ", &id);

    FILE *temp = fopen("temp.dat", "wb");
    if (!temp) { perror("Error opening temp file"); fclose(fp); return; }

    struct product p;
    while (fread(&p, sizeof(struct product), 1, fp) == 1) {
        if (p.id == id) {
            printf("Editing product %d...\n", id);
            read_line("Enter new name: ", p.name, sizeof(p.name));
            read_line("Enter new category: ", p.category, sizeof(p.category));
            read_int("Enter new quantity: ", &p.quantity);
            read_float("Enter new price: ", &p.price);
            read_line("Enter new supplier: ", p.supplier, sizeof(p.supplier));
            found = 1;
        }
        if (fwrite(&p, sizeof(struct product), 1, temp) != 1) {
            printf("Write failed during edit!\n");
            fclose(fp); fclose(temp);
            remove("temp.dat");
            return;
        }
    }
    fclose(fp);
    fclose(temp);

    if (remove(DATA_FILE) != 0 || rename("temp.dat", DATA_FILE) != 0) {
        perror("Failed to finalize edit");
        return;
    }

    if (found) printf("Product updated successfully!\n");
    else       printf("Product ID %d not found.\n", id);
}

void deleteproduct(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) { printf("No inventory data found.\n"); return; }

    int id, found = 0;
    read_int("Enter product ID to delete: ", &id);

    FILE *temp = fopen("temp.dat", "wb");
    if (!temp) { perror("Error opening temp file"); fclose(fp); return; }

    struct product p;
    while (fread(&p, sizeof(struct product), 1, fp) == 1) {
        if (p.id == id) {
            printf("Deleting product ID %d...\n", id);
            found = 1;
        } else {
            if (fwrite(&p, sizeof(struct product), 1, temp) != 1) {
                printf("Write failed during delete!\n");
                fclose(fp); fclose(temp);
                remove("temp.dat");
                return;
            }
        }
    }
    fclose(fp);
    fclose(temp);

    if (remove(DATA_FILE) != 0 || rename("temp.dat", DATA_FILE) != 0) {
        perror("Failed to finalize delete");
        return;
    }

    if (found) printf("Product deleted successfully!\n");
    else       printf("Product ID %d not found.\n", id);
}

void exportToCSV(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) { printf("No records found!\n"); return; }

    FILE *csv = fopen(CSV_FILE, "w");
    if (!csv) { perror("Error creating CSV file"); fclose(fp); return; }

    if (fprintf(csv, "ID,Name,Category,Quantity,Price,Supplier\n") < 0) {
        printf("Failed writing CSV header.\n");
        fclose(fp); fclose(csv); return;
    }

    struct product p;
    while (fread(&p, sizeof(struct product), 1, fp) == 1) {
        /* ID */
        fprintf(csv, "%d,", p.id);
        /* Name */
        csv_write_text(csv, p.name); fputc(',', csv);
        /* Category */
        csv_write_text(csv, p.category); fputc(',', csv);
        /* Quantity */
        fprintf(csv, "%d,", p.quantity);
        /* Price */
        fprintf(csv, "%.2f,", p.price);
        /* Supplier */
        csv_write_text(csv, p.supplier);
        fputc('\n', csv);
    }
    fclose(fp);
    if (fclose(csv) != 0) { perror("Error closing CSV file"); return; }

    printf("Inventory exported successfully to %s\n", CSV_FILE);
}

void lowStockReport(void) {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) { printf("No records found!\n"); return; }

    struct product p;
    int any = 0;

    printf("\n*** Low Stock Report (Quantity < 10) ***\n");
    printf("%-20s %-15s %-10s %-10s %-20s\n",
           "Name", "Category", "Quantity", "Price", "Supplier");
    printf("-------------------------------------------------------------------\n");

    while (fread(&p, sizeof(struct product), 1, fp) == 1) {
        if (p.quantity < 10) {
            any = 1;
            printf("%-20.20s %-15.15s %-10d %-10.2f %-20.20s\n",
                   p.name, p.category, p.quantity, p.price, p.supplier);
        }
    }
    if (!any) printf("All good. No low-stock items.\n");
    fclose(fp);
}

/* optional pause to keep output visible on some terminals */
static void pause_enter(void) {
    printf("\nPress Enter to continue...");
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

int main(void) {
    int choice;
    do {
        printf("=====================================================\n");
        printf("||                                                 ||\n");
        printf("||         WELCOME TO INVENTORY MANAGEMENT         ||\n");
        printf("||                                                 ||\n");
        printf("||      Manage Products | Suppliers | Reports      ||\n");
        printf("||                                                 ||\n        ");
        printf("=====================================================\n\n");

        printf("=== Inventory Management System ===\n");
        printf("1. Add Product\n");
        printf("2. View Products\n");
        printf("3. Search Product\n");
        printf("4. Edit Product\n");
        printf("5. Delete Product\n");
        printf("6. Low Stock Report\n");
        printf("7. Export To CSV\n");
        printf("8. Exit\n");
        printf("Enter choice: ");

        if (scanf("%d", &choice) != 1) {
            printf("Invalid choice! Try again.\n");
            flush_stdin_line();
            continue;
        }
        flush_stdin_line();

        switch (choice) {
            case 1: addproduct(); break;
            case 2: viewproduct(); break;
            case 3: searchproduct(); break;
            case 4: editproduct(); break;
            case 5: deleteproduct(); break;
            case 6: lowStockReport(); break;
            case 7: exportToCSV(); break;
            case 8: printf("Exiting program...\n"); break;
            default: printf("Invalid choice! Try again.\n");
        }
        if (choice != 8) pause_enter();
        printf("\n");
    } while (choice != 8);

    return 0;
}
