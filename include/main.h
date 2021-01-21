#define BUFFER_SIZE 128

#define GET_NUMBER(_str, _end_ptr) (_strtoui64(_str, _end_ptr, 10))

#define PML4_INDEX(logic_addr) ((logic_addr >> 39) & 0x1FF)
#define DIRECTORY_PTR_INDEX(logic_addr) ((logic_addr >> 30) & 0x1FF)
#define DIRECTORY_INDEX(logic_addr) ((logic_addr >> 21) & 0x1FF)
#define TABLE_INDEX(logic_addr) ((logic_addr >> 12) & 0x1FF)
#define OFFSET_INDEX(logic_addr) (logic_addr & 0xFFF)

#define GET_PHYS_ADDR_FROM_BASE(base, offset) (get_phys_addr(paddr_to_string((base + (offset * 8)))))
#define GET_PHYS_ADDR(base, offset) (get_phys_addr(paddr_to_string((base & 0xFFFFFFFFFF000) + (offset * 8))))
#define GET_PHYS_ADDR_FROM_PAGE_TABLE(base, offset) ((base & 0xFFFFFFFFFF000) + offset)

#define check_addr(phys_addr, file_out) \
        if (!phys_addr || !(phys_addr & 0x1)) { \
            fprintf(file_out, "fault\n"); \
            continue; \
        }                               \

#define check_phys_addr(phys_addr, file_out) \
        if (!phys_addr) { \
            fprintf(file_out, "fault\n"); \
            continue; \
        } \

// r - request, p - paddr -> value count, a -base address
struct rpa {
    uint64_t request_c;
    uint64_t paddr_value_c;
    uint64_t base_addr;
};

struct pair_paddr_by_value {
    char *paddr;
    uint64_t value;
};

HASHMAP(char, uint64_t) map;

struct rpa *init_rpa(FILE *file_in);

char *paddr_to_string(uint64_t value);

struct pair_paddr_by_value *create_pair_paddr_by_value(struct rpa *p_rpa, FILE *file_in);

uint64_t *create_request_array(struct rpa *p_rpa, FILE *file_in);

void fill_hashmap(struct rpa *rpa, struct pair_paddr_by_value *paddr_by_value);

uint64_t get_phys_addr(const char *paddr);

void page_proc(FILE *file_out, struct rpa *rpa, const uint64_t *request_array);