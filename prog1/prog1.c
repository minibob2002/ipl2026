#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int id;
	char name[50];
	int age;
	int deleted; /* 0 = present, 1 = deleted */
} Record;

void add_records(const char *filename, int n) {
	FILE *fp = fopen(filename, "ab");

	if (!fp) { perror("fopen"); return; }
	for (int i = 0; i < n; ++i) {
		Record r;
		printf("Record %d\n", i+1);
		printf("  id: "); if (scanf("%d", &r.id) != 1) { --i; fflush(stdin); continue; }
		printf("  name: "); scanf("%49s", r.name);
		printf("  age: "); scanf("%d", &r.age);
		r.deleted = 0;
		fwrite(&r, sizeof(Record), 1, fp);
	}
	fclose(fp);
}

int get_record(const char *filename, int m, Record *out) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) { perror("fopen"); return -1; }
	if (m <= 0) { fclose(fp); return -1; }
	if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
	long size = ftell(fp);
	if (size < 0) { fclose(fp); return -1; }
	long total = size / sizeof(Record);
	if (m > total) { fclose(fp); return 0; }
	long offset = (m - 1) * sizeof(Record);
	if (fseek(fp, offset, SEEK_SET) != 0) { fclose(fp); return -1; }
	if (fread(out, sizeof(Record), 1, fp) != 1) { fclose(fp); return -1; }
	fclose(fp);
	return 1;
}

int logical_delete(const char *filename, int m) {
	FILE *fp = fopen(filename, "r+b");
	if (!fp) { perror("fopen"); return -1; }
	if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
	long size = ftell(fp);
	long total = size / sizeof(Record);
	if (m <= 0 || m > total) { fclose(fp); return 0; }
	long offset = (m - 1) * sizeof(Record);
	if (fseek(fp, offset, SEEK_SET) != 0) { fclose(fp); return -1; }
	Record r;
	if (fread(&r, sizeof(Record), 1, fp) != 1) { fclose(fp); return -1; }
	if (r.deleted) { fclose(fp); return 2; }
	r.deleted = 1;
	if (fseek(fp, offset, SEEK_SET) != 0) { fclose(fp); return -1; }
	if (fwrite(&r, sizeof(Record), 1, fp) != 1) { fclose(fp); return -1; }
	fclose(fp);
	return 1;
}

int physical_delete(const char *filename, int m) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) { perror("fopen"); return -1; }
	if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
	long size = ftell(fp);
	long total = size / sizeof(Record);
	if (m <= 0 || m > total) { fclose(fp); return 0; }
	rewind(fp);
	FILE *tmp = fopen("tmp_records.dat", "wb");
	if (!tmp) { fclose(fp); perror("fopen tmp"); return -1; }
	Record r;
	long idx = 1;
	while (fread(&r, sizeof(Record), 1, fp) == 1) {
		if (idx != m && !r.deleted) fwrite(&r, sizeof(Record), 1, tmp);
		else if (idx != m && r.deleted) fwrite(&r, sizeof(Record), 1, tmp); /* keep already deleted records if you wish */
		idx++;
	}
	fclose(fp);
	fclose(tmp);
	if (remove(filename) != 0) { perror("remove"); return -1; }
	if (rename("tmp_records.dat", filename) != 0) { perror("rename"); return -1; }
	return 1;
}

void list_all(const char *filename) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) { printf("No records or cannot open file.\n"); return; }
	Record r;
	int i = 1;
	while (fread(&r, sizeof(Record), 1, fp) == 1) {
		printf("%d: id=%d name=%s age=%d %s\n", i, r.id, r.name, r.age, r.deleted?"(deleted)":"");
		i++;
	}
	fclose(fp);
}

int main(void) {
	const char *filename = "records.dat";
	int choice;
	for (;;) {
		printf("\nMenu:\n1) Add records\n2) Get m-th record\n3) Logical delete m-th record\n4) Physical delete m-th record\n5) List all\n6) Exit\nChoose: ");
		if (scanf("%d", &choice) != 1) break;
		if (choice == 1) {
			int n; printf("How many records to add? "); if (scanf("%d", &n) != 1) continue;
			add_records(filename, n);
		} else if (choice == 2) {
			int m; printf("Enter record number m: "); if (scanf("%d", &m) != 1) continue;
			Record r; int s = get_record(filename, m, &r);
			if (s == 1) {
				if (r.deleted) printf("Record %d is deleted.\n", m);
				else printf("Record %d: id=%d name=%s age=%d\n", m, r.id, r.name, r.age);
			} else if (s == 0) printf("No such record (index out of range).\n");
			else printf("Error reading file.\n");
		} else if (choice == 3) {
			int m; printf("Enter record number to logically delete: "); if (scanf("%d", &m) != 1) continue;
			int r = logical_delete(filename, m);
			if (r == 1) printf("Record %d marked deleted.\n", m);
			else if (r == 2) printf("Record %d was already deleted.\n", m);
			else if (r == 0) printf("No such record.\n");
			else printf("Error during deletion.\n");
		} else if (choice == 4) {
			int m; printf("Enter record number to physically delete: "); if (scanf("%d", &m) != 1) continue;
			int r = physical_delete(filename, m);
			if (r == 1) printf("Record %d physically removed.\n", m);
			else if (r == 0) printf("No such record.\n");
			else printf("Error during physical deletion.\n");
		} else if (choice == 5) {
			list_all(filename);
		} else break;
	}
	return 0;
}

