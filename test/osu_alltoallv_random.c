#define BENCHMARK "OSU MPI%s All-to-Allv Personalized Exchange Latency Test"

/*                          COPYRIGHT
 *
 * Copyright (c) 2001-2023, The Ohio State University. All rights
 * reserved.
 *
 * The OMB (OSU Micro Benchmarks) software package is developed by the team
 * members of The Ohio State University's Network-Based Computing Laboratory
 * (NBCL), headed by Professor Dhabaleswar K. (DK) Panda.
 *
 * Contact:
 * Prof. Dhabaleswar K. (DK) Panda
 * Dept. of Computer Science and Engineering
 * The Ohio State University
 * 2015 Neil Avenue
 * Columbus, OH - 43210-1277
 * Tel: (614)-292-5199; Fax: (614)-292-2911
 * E-mail:panda@cse.ohio-state.edu
 *
 * This program is available under BSD licensing.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * (1) Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * (2) Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * (3) Neither the name of The Ohio State University nor the names of
 * their contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../MPI_Alltoallv_custom.h"
#include "osu_util_mpi.h"

int main(int argc, char *argv[])
{
  int i = 0, j, rank = 0, size, numprocs, disp;
  double latency = 0.0, t_start = 0.0, t_stop = 0.0;
  double timer = 0.0;
  int errors = 0, local_errors = 0;
  double avg_time = 0.0, max_time = 0.0, min_time = 0.0;
  char *sendbuf = NULL, *recvbuf = NULL;
  int *rdispls = NULL, *recvcounts = NULL, *sdispls = NULL, *sendcounts = NULL;
  void *sendbuf_warmup = NULL, *recvbuf_warmup = NULL;
  int po_ret;
  size_t bufsize;
  omb_graph_options_t omb_graph_options;
  omb_graph_data_t *omb_graph_data = NULL;
  int papi_eventset                = OMB_PAPI_NULL;
  options.bench                    = COLLECTIVE;
  options.subtype                  = ALLTOALL;
  size_t num_elements              = 0;
  MPI_Datatype omb_curr_datatype   = MPI_CHAR;
  size_t omb_ddt_transmit_size     = 0;
  int mpi_type_itr = 0, mpi_type_size = 0, mpi_type_name_length = 0;
  char mpi_type_name_str[OMB_DATATYPE_STR_MAX_LEN];
  MPI_Datatype mpi_type_list[OMB_NUM_DATATYPES];
  MPI_Comm omb_comm = MPI_COMM_NULL;
  omb_mpi_init_data omb_init_h;
  struct omb_buffer_sizes_t omb_buffer_sizes;

  set_header(HEADER);
  set_benchmark_name("osu_alltoallv");
  po_ret = process_options(argc, argv);
  omb_populate_mpi_type_list(mpi_type_list);

  if(PO_OKAY == po_ret && NONE != options.accel)
    {
      if(init_accel())
        {
          fprintf(stderr, "Error initializing device\n");
          exit(EXIT_FAILURE);
        }
    }

  omb_init_h = omb_mpi_init(&argc, &argv);
  omb_comm   = omb_init_h.omb_comm;
  if(MPI_COMM_NULL == omb_comm)
    {
      OMB_ERROR_EXIT("Cant create communicator");
    }
  MPI_CHECK(MPI_Comm_rank(omb_comm, &rank));
  MPI_CHECK(MPI_Comm_size(omb_comm, &numprocs));

  omb_graph_options_init(&omb_graph_options);
  switch(po_ret)
    {
      case PO_BAD_USAGE:
        print_bad_usage_message(rank);
        omb_mpi_finalize(omb_init_h);
        exit(EXIT_FAILURE);
      case PO_HELP_MESSAGE:
        print_help_message(rank);
        omb_mpi_finalize(omb_init_h);
        exit(EXIT_SUCCESS);
      case PO_VERSION_MESSAGE:
        print_version_message(rank);
        omb_mpi_finalize(omb_init_h);
        exit(EXIT_SUCCESS);
      case PO_OKAY:
        break;
    }

  if(numprocs < 2)
    {
      if(rank == 0)
        {
          fprintf(stderr, "This test requires at least two processes\n");
        }
      omb_mpi_finalize(omb_init_h);
      exit(EXIT_FAILURE);
    }
  check_mem_limit(numprocs);
  if(allocate_memory_coll((void **)&recvcounts, numprocs * sizeof(int), NONE))
    {
      fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
      MPI_CHECK(MPI_Abort(omb_comm, EXIT_FAILURE));
    }
  if(allocate_memory_coll((void **)&sendcounts, numprocs * sizeof(int), NONE))
    {
      fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
      MPI_CHECK(MPI_Abort(omb_comm, EXIT_FAILURE));
    }

  if(allocate_memory_coll((void **)&rdispls, numprocs * sizeof(int), NONE))
    {
      fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
      MPI_CHECK(MPI_Abort(omb_comm, EXIT_FAILURE));
    }
  if(allocate_memory_coll((void **)&sdispls, numprocs * sizeof(int), NONE))
    {
      fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
      MPI_CHECK(MPI_Abort(omb_comm, EXIT_FAILURE));
    }

  bufsize = 2 * options.max_message_size * numprocs;
  if(allocate_memory_coll((void **)&sendbuf, bufsize, options.accel))
    {
      fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
      MPI_CHECK(MPI_Abort(omb_comm, EXIT_FAILURE));
    }
  set_buffer(sendbuf, options.accel, 1, bufsize);
  sendbuf_warmup                = sendbuf;
  omb_buffer_sizes.sendbuf_size = bufsize;

  if(allocate_memory_coll((void **)&recvbuf, bufsize, options.accel))
    {
      fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
      MPI_CHECK(MPI_Abort(omb_comm, EXIT_FAILURE));
    }
  set_buffer(recvbuf, options.accel, 0, bufsize);
  omb_buffer_sizes.recvbuf_size = bufsize;
  if(allocate_memory_coll((void **)&recvbuf_warmup, bufsize, options.accel))
    {
      fprintf(stderr, "Could Not Allocate Memory [rank %d]\n", rank);
      MPI_CHECK(MPI_Abort(omb_comm, EXIT_FAILURE));
    }
  set_buffer(recvbuf_warmup, options.accel, 0, bufsize);

  print_preamble(rank);
  omb_papi_init(&papi_eventset);

  srand(rank * 3421);

  if(rank == 0)
    {
      printf("# use_custom = %d\n", options.use_custom);
    }

  if(options.validate == 1)
    {
      options.validate = 0;
      if(rank == 0)
        {
          printf("# Validation is not supported for this test\n");
        }
    }

  MPI_CHECK(MPI_Barrier(omb_comm));
  for(mpi_type_itr = 0; mpi_type_itr < options.omb_dtype_itr; mpi_type_itr++)
    {
      MPI_CHECK(MPI_Type_size(mpi_type_list[mpi_type_itr], &mpi_type_size));
      MPI_CHECK(MPI_Type_get_name(mpi_type_list[mpi_type_itr], mpi_type_name_str, &mpi_type_name_length));
      omb_curr_datatype = mpi_type_list[mpi_type_itr];
      OMB_MPI_RUN_AT_RANK_ZERO(fprintf(stdout, "# Datatype: %s.\n", mpi_type_name_str));
      fflush(stdout);
      print_only_header(rank);
      for(size = options.min_message_size; size <= options.max_message_size; size *= 2)
        {
          num_elements = size / mpi_type_size;
          if(0 == num_elements)
            {
              continue;
            }
          if(size > LARGE_MESSAGE_SIZE)
            {
              options.skip       = options.skip_large;
              options.iterations = options.iterations_large;
            }

          omb_ddt_transmit_size = omb_ddt_assign(&omb_curr_datatype, mpi_type_list[mpi_type_itr], num_elements) * mpi_type_size;
          num_elements          = omb_ddt_get_size(num_elements);
          MPI_CHECK(MPI_Barrier(omb_comm));

          omb_graph_allocate_and_get_data_buffer(&omb_graph_data, &omb_graph_options, size, options.iterations);
          MPI_CHECK(MPI_Barrier(omb_comm));
          timer = 0.0;
          for(i = 0; i < options.iterations + options.skip; i++)
            {
              disp = 0;
              for(j = 0; j < numprocs; j++)
                {
                  sendcounts[j] = (int)(2 * ((double)rand() / RAND_MAX) * num_elements);
                  sdispls[j]    = disp;
                  disp += sendcounts[j];
                }
              MPI_CHECK(MPI_Alltoall(sendcounts, 1, MPI_INT, recvcounts, 1, MPI_INT, omb_comm));
              disp = 0;
              for(j = 0; j < numprocs; j++)
                {
                  rdispls[j] = disp;
                  disp += recvcounts[j];
                }

              MPI_CHECK(MPI_Barrier(omb_comm));

              if(i == options.skip)
                {
                  omb_papi_start(&papi_eventset);
                }
              if(options.validate)
                {
                  set_buffer_validation(sendbuf, recvbuf, size, options.accel, i, omb_curr_datatype, omb_buffer_sizes);
                  if(1 == options.omb_enable_mpi_in_place)
                    {
                      sendbuf = MPI_IN_PLACE;
                    }
                  for(j = 0; j < options.warmup_validation; j++)
                    {
                      MPI_CHECK(MPI_Barrier(omb_comm));
                      if(options.use_custom == 1)
                        MPI_CHECK(MPI_Alltoallv_custom(sendbuf_warmup, sendcounts, sdispls, omb_curr_datatype, recvbuf_warmup,
                                                       recvcounts, rdispls, omb_curr_datatype, omb_comm));
                      else if(options.use_custom == 2)
                        MPI_CHECK(MPI_Alltoallv_custom_shared(sendbuf_warmup, sendcounts, sdispls, omb_curr_datatype, recvbuf_warmup,
                                                              recvcounts, rdispls, omb_curr_datatype, omb_comm));
                      else
                        MPI_CHECK(MPI_Alltoallv(sendbuf_warmup, sendcounts, sdispls, omb_curr_datatype, recvbuf_warmup, recvcounts,
                                                rdispls, omb_curr_datatype, omb_comm));
                    }
                  MPI_CHECK(MPI_Barrier(omb_comm));
                }

              t_start = MPI_Wtime();

              if(options.use_custom == 1)
                MPI_CHECK(MPI_Alltoallv_custom(sendbuf, sendcounts, sdispls, omb_curr_datatype, recvbuf, recvcounts, rdispls,
                                               omb_curr_datatype, omb_comm));
              else if(options.use_custom == 2)
                MPI_CHECK(MPI_Alltoallv_custom_shared(sendbuf, sendcounts, sdispls, omb_curr_datatype, recvbuf, recvcounts, rdispls,
                                                      omb_curr_datatype, omb_comm));
              else
                MPI_CHECK(MPI_Alltoallv(sendbuf, sendcounts, sdispls, omb_curr_datatype, recvbuf, recvcounts, rdispls,
                                        omb_curr_datatype, omb_comm));

              t_stop = MPI_Wtime();

              MPI_CHECK(MPI_Barrier(omb_comm));

              if(options.validate)
                {
                  local_errors += validate_data(recvbuf, size, numprocs, options.accel, i, omb_curr_datatype);
                }

              if(i >= options.skip)
                {
                  timer += t_stop - t_start;
                  if(options.graph && 0 == rank)
                    {
                      omb_graph_data->data[i - options.skip] = (t_stop - t_start) * 1e6;
                    }
                }
            }
          omb_papi_stop_and_print(&papi_eventset, size);

          latency = (double)(timer * 1e6) / options.iterations;

          MPI_CHECK(MPI_Reduce(&latency, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, omb_comm));
          MPI_CHECK(MPI_Reduce(&latency, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, omb_comm));
          MPI_CHECK(MPI_Reduce(&latency, &avg_time, 1, MPI_DOUBLE, MPI_SUM, 0, omb_comm));
          avg_time = avg_time / numprocs;

          if(options.validate)
            {
              MPI_CHECK(MPI_Allreduce(&local_errors, &errors, 1, MPI_INT, MPI_SUM, omb_comm));
            }

          if(options.validate)
            {
              print_stats_validate(rank, size, avg_time, min_time, max_time, errors);
            }
          else
            {
              print_stats(rank, size, avg_time, min_time, max_time);
            }
          if(options.graph && 0 == rank)
            {
              omb_graph_data->avg = avg_time;
            }
          omb_ddt_append_stats(omb_ddt_transmit_size);
          omb_ddt_free(&omb_curr_datatype);
          MPI_CHECK(MPI_Barrier(omb_comm));

          if(0 != errors)
            {
              break;
            }
        }
    }
  if(0 == rank && options.graph)
    {
      omb_graph_plot(&omb_graph_options, benchmark_name);
    }
  omb_graph_combined_plot(&omb_graph_options, benchmark_name);
  omb_graph_free_data_buffers(&omb_graph_options);
  omb_papi_free(&papi_eventset);

  free_buffer(rdispls, NONE);
  free_buffer(sdispls, NONE);
  free_buffer(recvcounts, NONE);
  free_buffer(sendcounts, NONE);
  free_buffer(sendbuf_warmup, options.accel);
  free_buffer(recvbuf_warmup, options.accel);
  free_buffer(recvbuf, options.accel);
  omb_mpi_finalize(omb_init_h);

  if(NONE != options.accel)
    {
      if(cleanup_accel())
        {
          fprintf(stderr, "Error cleaning up device\n");
          exit(EXIT_FAILURE);
        }
    }

  if(0 != errors && options.validate && 0 == rank)
    {
      fprintf(stdout,
              "DATA VALIDATION ERROR: %s exited with status %d on"
              " message size %d.\n",
              argv[0], EXIT_FAILURE, size);
      exit(EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}

/* vi: set sw=4 sts=4 tw=80: */
