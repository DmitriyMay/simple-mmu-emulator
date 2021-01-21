#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/hashmap.h"
#include "../include/main.h"

static void check_arg_count(int arg) {
    if (arg < 3) {
        printf("You must transfer file name");
        exit(-1);
    }
}

static void check_file(FILE *file_in, const char *filename_in) {
    if (!file_in) {
        printf("Not open file %s", filename_in);
        exit(-1);
    }
}

int main(int arg, const char *argv[]) {
    check_arg_count(arg);
    const char *filename_in = argv[1];
    const char *filename_out = argv[2];
    FILE *file_in = fopen(filename_in, "r");
    check_file(file_in, filename_in);
    struct rpa *rpa = init_rpa(file_in);
    struct pair_paddr_by_value *paddr_by_value = create_pair_paddr_by_value(rpa, file_in);
    uint64_t *request_array = create_request_array(rpa, file_in);
    hashmap_init(&map, hashmap_hash_string, strcmp);
    fill_hashmap(rpa, paddr_by_value);
    FILE *file_out = fopen(filename_out, "w");
    check_file(file_out, filename_out);
    page_proc(file_out, rpa, request_array);
    fclose(file_in);
    fclose(file_out);
    return 0;
}

/*
 * init pra structure this saving:
 * paddr_value_c - count for mapping phys_addr - value(phys_addr)
 * request_c - count all request for resolve logic addr in phys addr
 * base_addr - emulation cr3 register(store base phys addr for paging table)
 */
struct rpa *init_rpa(FILE *file_in) {
    struct rpa *rpa = malloc(sizeof(struct rpa));
    char buffer[BUFFER_SIZE];
    fgets(buffer, BUFFER_SIZE, file_in);
    char *m, *q, *r;
    rpa->paddr_value_c = strtoull(buffer, &m, 10);
    rpa->request_c = strtoull(m, &q, 10);
    rpa->base_addr = strtoull(q, &r, 10);
    return rpa;
}

struct pair_paddr_by_value *create_pair_paddr_by_value(struct rpa *p_rpa, FILE *file_in) {
    struct pair_paddr_by_value *paddr_by_value = malloc(sizeof(struct pair_paddr_by_value) * p_rpa->paddr_value_c);
    int i;
    for (i = 0; i < p_rpa->paddr_value_c; i++) {
        uint64_t paddr = 0;
        uint64_t value;
        fscanf(file_in, "%llu %llu", &paddr, &value);
        paddr_by_value[i].paddr = paddr_to_string(paddr);
        paddr_by_value[i].value = value;
    }
    return paddr_by_value;
}

char *paddr_to_string(uint64_t value) {
    int length = snprintf(NULL, 0, "%llu", value);
    char *p_to_string = malloc(length + 1);
    snprintf(p_to_string, length + 1, "%llu", value);
    return p_to_string;
}

uint64_t *create_request_array(struct rpa *p_rpa, FILE *file_in) {
    int i;
    char buffer[BUFFER_SIZE];
    char *end_ptr;
    uint64_t *request_array = malloc(sizeof(uint64_t) * p_rpa->request_c);
    for (i = 0; i < p_rpa->request_c && fgets(buffer, BUFFER_SIZE, file_in); i++) {
        uint64_t logic_addr = GET_NUMBER(buffer, &end_ptr);
        request_array[i] = logic_addr;
    }
    return request_array;
}

void fill_hashmap(struct rpa *rpa, struct pair_paddr_by_value *paddr_by_value) {
    int i;
    int r;
    for (i = 0; i < rpa->paddr_value_c; i++) {
        char *paddr = paddr_by_value[i].paddr;
        uint64_t *value = malloc(sizeof(uint64_t));
        *value = paddr_by_value[i].value;
        r = hashmap_put(&map, paddr, value);
        if (r < 0) {
            /* Expect -EEXIST return value for duplicates */
            printf("putting paddr[%s] failed: %s\n", paddr_by_value->paddr, strerror(-r));
        }
    }
}

uint64_t get_phys_addr(const char *paddr) {
    uint64_t *value = (uint64_t *) hashmap_get(&map, paddr);
    if (!value) {
        return 0ll;
    }
    return *value;
}

void page_proc(FILE *file_out, struct rpa *rpa, const uint64_t *request_array) {
    int i;
    for (i = 0; i < rpa->request_c; i++) {
        uint64_t logic_addr = request_array[i];
        // emulation processing paging x86 long mod
        //resolve pml4e
        uint64_t pml4 = PML4_INDEX(logic_addr);
        uint64_t pml4e = GET_PHYS_ADDR_FROM_BASE(rpa->base_addr, pml4);
        check_addr(pml4e, file_out)
        //resolve pdte
        uint64_t directory_ptr = DIRECTORY_PTR_INDEX(logic_addr);
        uint64_t pdte = GET_PHYS_ADDR(pml4e, directory_ptr);
        check_addr(pdte, file_out)
        // resolve pde
        uint64_t directory = DIRECTORY_INDEX(logic_addr);
        uint64_t pde = GET_PHYS_ADDR(pdte, directory);
        check_addr(pde, file_out)
        // resolve pte
        uint64_t table = TABLE_INDEX(logic_addr);
        uint64_t pte = GET_PHYS_ADDR(pde, table);
        check_addr(pte, file_out)
        // resolve phys addr from page table
        uint64_t offset = OFFSET_INDEX(logic_addr);
        uint64_t phys_addr = GET_PHYS_ADDR_FROM_PAGE_TABLE(pte, offset);
        check_phys_addr(phys_addr, file_out)
        fprintf(file_out, "%lld\n", phys_addr);
    }
}
