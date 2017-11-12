#include "server_bck.hh"
#include "process_operation.hh"

// Handling incoming requests on backend (Function ultimately calls one of the functions mentioned below)
request process_request(const request& req_payload);

// Handling GET request on backend
request process_get_req(const request& req_payload);

// Handling PUT request on backend
request process_put_req(const request& req_payload);

// Handling DELETE request on backend
request process_delete_req(const request& req_payload);

// Handling CPUT request on backend
request process_cput_req(const request& req_payload);

// Handling GET_COLS request on backend
request process_get_cols_req(const request& req_payload);

// Handling GET_ROWS request on backend
request process_get_rows_req(const request& req_payload);

