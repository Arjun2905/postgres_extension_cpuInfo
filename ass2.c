#include <stdio.h>
#include <stdlib.h>
#include "postgres.h"
#include <string.h>
#include "miscadmin.h"
#include "fmgr.h"
#include "funcapi.h"
#include <utils/builtins.h>

#define SIZE 5
#define IS_EMPTY_STR(X) ((1 / (sizeof(X[0]) == 1)) && !(X[0]))
#define CPU_VENDOR 0
#define MODEL_NAME 1
#define PHY_PROCESSOR 2
#define CORES 3
#define CLOCK_SPEED 4
PG_MODULE_MAGIC;

void read_cpu_info(Tuplestorestate *tupstore, TupleDesc tupdesc);
char *leftTrimStr(char *s);
char *rightTrimStr(char *s);
char *trimStr(char *s);

PG_FUNCTION_INFO_V1(ass2);

char *leftTrimStr(char *s)
{
    char *new_str = s;
    while (isspace(*new_str))
    {
        ++new_str;
    }
    memmove(s, new_str, strlen(new_str) + 1);
    return s;
}

char *rightTrimStr(char *s)
{
    char *end = s + strlen(s);
    while ((end != s) && isspace(*(end - 1)))
    {
        --end;
    }
    *end = '\0';
    return s;
}

char *trimStr(char *s)
{
    return rightTrimStr(leftTrimStr(s));
}

void read_cpu_info(Tuplestorestate *tupstore, TupleDesc tupdesc)
{
    char *found;
    FILE *file;
    Datum values[SIZE];
    bool nulls[SIZE];
    char vendor_id[MAXPGPATH];
    char model[MAXPGPATH];
    char model_name[MAXPGPATH];
    char cpu_mhz[MAXPGPATH];
    char *line = NULL;
    size_t line_buf_size = 0;
    ssize_t line_size;
    bool model_found = false;

    int physical_processor = 0;

    int cpu_cores = 0;
    float cpu_hz;
    uint64_t cpu_freq;

    memset(nulls, 0, sizeof(nulls));
    memset(vendor_id, 0, MAXPGPATH);
    memset(model, 0, MAXPGPATH);
    memset(model_name, 0, MAXPGPATH);
    memset(cpu_mhz, 0, MAXPGPATH);
    file = fopen("/proc/cpuinfo", "r");
    line_size = getline(&line, &line_buf_size, file);

    while (line_size >= 0)
    {
        if (strlen(line) > 0)
            line = trimStr(line);

        if (!IS_EMPTY_STR(line) && (strlen(line) > 0))
        {
            if (strlen(line) > 0)
            {
                found = strstr(line, ":");
                if (strlen(found) > 0)
                {
                    found = trimStr((found + 1));

                    if (strstr(line, "vendor_id") != NULL)
                        memcpy(vendor_id, found, strlen(found));
                    if (strstr(line, "model") != NULL && !model_found)
                    {
                        memcpy(model, found, strlen(found));
                        model_found = true;
                    }
                    if (strstr(line, "model name") != NULL)
                        memcpy(model_name, found, strlen(found));
                    if (strstr(line, "cpu MHz") != NULL)
                    {
                        physical_processor++;
                        memcpy(cpu_mhz, found, strlen(found));
                    }
                    if (strstr(line, "cpu cores") != NULL)
                        cpu_cores = atoi(found);
                }
            }

            if (line != NULL)
            {
                free(line);
                line = NULL;
            }
        }
        line_size = getline(&line, &line_buf_size, file);
    }

    if (line != NULL)
    {
        free(line);
        line = NULL;
    }

    fclose(file);

    if (physical_processor)
    {
        cpu_hz = atof(cpu_mhz);
        cpu_freq = (cpu_hz);
        values[CPU_VENDOR] = CStringGetTextDatum(vendor_id);
        values[MODEL_NAME] = CStringGetTextDatum(model_name);
        values[PHY_PROCESSOR] = Int32GetDatum(physical_processor);
        values[CORES] = Int32GetDatum(cpu_cores);
        values[CLOCK_SPEED] = Int64GetDatumFast(cpu_freq);
        tuplestore_putvalues(tupstore, tupdesc, values, nulls);
    }
}

Datum ass2(PG_FUNCTION_ARGS)
{
    ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
    TupleDesc tupdesc;
    Tuplestorestate *tupstore;
    MemoryContext per_query_ctx;
    MemoryContext oldcontext;
    per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
    oldcontext = MemoryContextSwitchTo(per_query_ctx);

    if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
        elog(ERROR, "return type must be a row type");

    Assert(tupdesc->natts == SIZE);

    tupstore = tuplestore_begin_heap(true, false, work_mem);
    rsinfo->returnMode = SFRM_Materialize;
    rsinfo->setResult = tupstore;
    rsinfo->setDesc = tupdesc;

    MemoryContextSwitchTo(oldcontext);
    read_cpu_info(tupstore, tupdesc);
    tuplestore_donestoring(tupstore);
    return (Datum)0;
}