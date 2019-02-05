	// Requests
	{
        int r = -1;
        int r1 = -1;
        int r2 = -1;
        int r3 = -1;
        mcapi_requests* requests = &mcapi_rq;
        indexed_array_header* header = &requests->reserves_header;

        mcapi_trans_init_indexed_array();
        assert(MCAPI_MAX_REQUESTS == header->max_count);
        assert(0 == header->curr_count);

        r = mcapi_trans_request_get_index();
        assert(-1 != r);
        assert(mcapi_trans_request_release_index(r));

        assert(mcapi_trans_reserve_request(&r1));
        assert(REQUEST_VALID == requests->data[r1].state);
        assert(1 == header->curr_count);

        assert(mcapi_trans_reserve_request(&r2));
        assert(REQUEST_VALID == requests->data[r2].state);
        assert(2 == header->curr_count);

        assert(mcapi_trans_reserve_request(&r3));
        assert(REQUEST_VALID == requests->data[r3].state);
        assert(3 == header->curr_count);

        // remove the middle request
        requests->data[r2].state = REQUEST_COMPLETED;
        assert(mcapi_trans_remove_request(r2));
        assert(REQUEST_FREE == requests->data[r2].state);
        assert(2 == header->curr_count);

        // remove the first remaining request
        requests->data[r3].state = REQUEST_COMPLETED;
        assert(mcapi_trans_remove_request(r3));
        assert(REQUEST_FREE == requests->data[r3].state);
        assert(1 == header->curr_count);

        // remove the last remaining request
        requests->data[r1].state = REQUEST_COMPLETED;
        assert(mcapi_trans_remove_request(r1));
        assert(REQUEST_FREE == requests->data[r1].state);
        assert(0 == header->curr_count);
	}
