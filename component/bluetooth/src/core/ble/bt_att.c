#include "bt_att.h"

att_cbs_t *att_cbs;

l2cap_pcb_t *att_l2cap;

static err_t att_send_data(struct bt_pbuf_t *p);
err_t att_connect_ind(void *arg, l2cap_pcb_t *pcb, err_t err);
err_t att_disconnect_ind(void *arg, l2cap_pcb_t *pcb, err_t err);
err_t att_input(void *arg, l2cap_pcb_t *l2cappcb, struct bt_pbuf_t *p, err_t err);




err_t att_connect_ind(void *arg, l2cap_pcb_t *pcb, err_t err)
{
	BT_ATT_TRACE_DEBUG("att_connect_ind\n");
	return BT_ERR_OK;
}

err_t att_disconnect_ind(void *arg, l2cap_pcb_t *pcb, err_t err)
{
	BT_ATT_TRACE_DEBUG("att_disconnect_ind\n");
	l2cap_fixed_channel_register_recv(L2CAP_ATT_CID,att_connect_ind,att_disconnect_ind,att_input);
	return BT_ERR_OK;
}

err_t att_input(void *arg, l2cap_pcb_t *l2cappcb, struct bt_pbuf_t *p, err_t err)
{
    BT_ATT_TRACE_DEBUG("att_input\n");

    bt_hex_dump(p->payload,p->len);
    att_l2cap = l2cappcb;
    if(att_cbs && att_cbs->att_data_recv)
        att_cbs->att_data_recv(NULL,p);

    return BT_ERR_OK;
}


err_t att_init(void)
{
    l2cap_fixed_channel_register_recv(L2CAP_ATT_CID,att_connect_ind,att_disconnect_ind,att_input);

    return BT_ERR_OK;
}

err_t att_register_cb(att_cbs_t *cb)
{
    att_cbs = cb;

    return BT_ERR_OK;
}

err_t att_err_rsp(uint8_t req_op,uint16_t handle,uint8_t err_code)
{
    struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_ERR_RSP_PACK_LEN, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_ERROR;
    ((uint8_t *)send_pbuf->payload)[1] = req_op;
    bt_le_store_16((uint8_t *)send_pbuf->payload,2,handle);
    ((uint8_t *)send_pbuf->payload)[4] = err_code;

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}



err_t att_parse_mtu_req(struct bt_pbuf_t *p,uint16_t *client_mtu)
{
    uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_mtu_req data_len(%d)\n",data_len);
    *client_mtu = bt_le_read_16(data,1);

    return BT_ERR_OK;
}


err_t att_mtu_rsp(uint16_t server_mtu)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_EXCHANGE_MTU_PACK_LEN, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_MTU;
    bt_le_store_16((uint8_t *)send_pbuf->payload,1,server_mtu);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}


err_t att_parse_find_info_req(struct bt_pbuf_t *p,uint16_t *s_handle,uint16_t *e_handle)
{
    uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_find_info_req data_len(%d)\n",data_len);
    *s_handle = bt_le_read_16(data,1);
    *e_handle = bt_le_read_16(data,3);

    return BT_ERR_OK;
}



err_t att_find_info_rsp(uint8_t uuid_format,uint8_t *info_data,uint8_t info_len)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_FIND_INFO_RSP_HDR_LEN+info_len, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_FIND_INFO;
	((uint8_t *)send_pbuf->payload)[1] = uuid_format;
    memcpy(((uint8_t *)send_pbuf->payload)+2, info_data, info_len);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}

err_t att_find_info_value_type_rsp(uint8_t uuid_format,uint16_t found_handle,uint16_t end_group_handle)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_FIND_INFO_VALUE_TYPE_RSP_PACK_LEN, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_FIND_TYPE_VALUE;
	bt_le_store_16((uint8_t *)send_pbuf->payload,1,found_handle);
	bt_le_store_16((uint8_t *)send_pbuf->payload,3,end_group_handle);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}

err_t att_read_type_rsp(uint8_t *data_list,uint8_t data_list_len)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_READ_TYPE_RSP_PACK_LEN+data_list_len, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_READ_BY_TYPE;
	((uint8_t *)send_pbuf->payload)[1] = data_list_len;
    memcpy(((uint8_t *)send_pbuf->payload)+2, data_list, data_list_len);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}

err_t att_read_blob_rsp(uint8_t *att_value,uint8_t att_value_len)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_READ_BLOB_RSP_HDR_LEN+att_value_len, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_READ_BLOB;
    memcpy(((uint8_t *)send_pbuf->payload)+1, att_value, att_value_len);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}


err_t att_read_multi_rsp(void)
{
	return BT_ERR_OK;
}


err_t att_read_rsp(uint8_t *att_value,uint8_t att_value_len)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_READ_RSP_HDR_LEN+att_value_len, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_READ;
    memcpy(((uint8_t *)send_pbuf->payload)+1, att_value, att_value_len);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}

err_t att_read_group_type_rsp(uint8_t *att_dlist,uint8_t att_dlist_len)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_READ_GROUP_TYPE_RSP_HDR_LEN+att_dlist_len, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_READ_BY_GRP_TYPE;
	((uint8_t *)send_pbuf->payload)[1] = att_dlist_len;
    memcpy(((uint8_t *)send_pbuf->payload)+2, att_dlist, att_dlist_len);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
	
}


err_t att_parse_multi_var_req(void)
{
	return BT_ERR_OK;
}

err_t att_multi_var_rsp(void)
{
	return BT_ERR_OK;
}


err_t att_write_rsp(void)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_WRITE_RSP_PACK_LEN, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }
    ((uint8_t *)send_pbuf->payload)[0] = ATT_RSP_WRITE;

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);
	
	return BT_ERR_OK;
}

err_t att_notification(uint16_t handle,uint8_t *att_value,uint8_t att_value_len)
{
	struct bt_pbuf_t *send_pbuf;
	if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_SEND_NOTIFICATION_HDR_LEN+att_value_len, BT_PBUF_RAM)) == NULL)
    {
        BT_GATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_HANDLE_VALUE_NOTIF;
    bt_le_store_16((uint8_t *)send_pbuf->payload,1,handle);
	memcpy(((uint8_t *)send_pbuf->payload)+3, att_value, att_value_len);
	
    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);
		
		return BT_ERR_OK;
}

err_t att_indication(uint16_t handle,uint8_t *att_value,uint8_t att_value_len)
{
	struct bt_pbuf_t *send_pbuf;
	if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_SEND_INDICATION_HDR_LEN+att_value_len, BT_PBUF_RAM)) == NULL)
    {
        BT_GATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_HANDLE_VALUE_IND;
    bt_le_store_16((uint8_t *)send_pbuf->payload,1,handle);
	memcpy(((uint8_t *)send_pbuf->payload)+3, att_value, att_value_len);
	
    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);
		
		return BT_ERR_OK;
}



err_t att_parse_read_req(struct bt_pbuf_t *p,uint16_t *handle)
{
    uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_read_req data_len(%d)\n",data_len);
    *handle = bt_le_read_16(data,1);

    return BT_ERR_OK;
}




err_t att_parse_find_info_type_value_req(struct bt_pbuf_t *p,uint16_t *s_handle,uint16_t *e_handle,uint16_t *att_type,uint8_t *value,uint8_t *value_len)
{
	uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_find_info_type_value_req data_len(%d)\n",data_len);
    *s_handle = bt_le_read_16(data,1);
    *e_handle = bt_le_read_16(data,3);
	*att_type = bt_le_read_16(data,5);

	*value_len = data_len-7;
    if (*value_len > GATT_BLE_MTU_SIZE)
        *value_len =  GATT_BLE_MTU_SIZE;
	memcpy(value,data+7,*value_len);

    return BT_ERR_OK;
}


err_t att_parse_read_group_type_req(struct bt_pbuf_t *p,uint16_t *s_handle,uint16_t *e_handle,uint16_t *uuid)
{
    uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_read_group_type_req data_len(%d)\n",data_len);
    *s_handle = bt_le_read_16(data,1);
    *e_handle = bt_le_read_16(data,3);

    if(data_len ==  7)
        *uuid = bt_le_read_16(data,5);
    else
    {
        /* TODO:UUID128->UUID16 */
    }


    return BT_ERR_OK;
}

err_t att_parse_write_req(struct bt_pbuf_t *p,uint16_t *handle,uint8_t *att_value,uint8_t *value_len)
{
	uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_write_req data_len(%d)\n",data_len);
    *handle = bt_le_read_16(data,1);

	*value_len = data_len-3;
    if (*value_len > GATT_BLE_MTU_SIZE)
        *value_len = GATT_BLE_MTU_SIZE;

	memcpy(att_value,data+3,*value_len);

    return BT_ERR_OK;
}

err_t att_parse_write_cmd(struct bt_pbuf_t *p,uint16_t *handle,uint8_t *att_value,uint8_t *value_len)
{
	uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_write_cmd data_len(%d)\n",data_len);
    *handle = bt_le_read_16(data,1);

	*value_len = data_len-3;
    if (*value_len > GATT_BLE_MTU_SIZE)
        *value_len = GATT_BLE_MTU_SIZE;
        
	memcpy(att_value,data+7,*value_len);

    return BT_ERR_OK;
}

err_t att_parse_sig_write_cmd(void)
{
	return BT_ERR_OK;
}

err_t att_parse_pre_write_req(void)
{
	return BT_ERR_OK;
}
err_t att_pre_write_rsp(void)
{
	return BT_ERR_OK;
}

err_t att_parse_exc_write_req(void)
{
	return BT_ERR_OK;
}
err_t att_exc_write_rsp(void)
{
	return BT_ERR_OK;
}


err_t att_parse_read_type_req(struct bt_pbuf_t *p,uint16_t *s_handle,uint16_t *e_handle,
	uint8_t *uuid_format,uint16_t *uuid,uint8_t **uuid128)
{
    uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_read_type_req data_len(%d)\n",data_len);
    *s_handle = bt_le_read_16(data,1);
    *e_handle = bt_le_read_16(data,3);

    if(data_len ==  7)
    {
    	*uuid_format = ATT_UUID16_FORMAT;
        *uuid = bt_le_read_16(data,5);
    }
    else
    {
    	*uuid_format = ATT_UUID128_FORMAT;
		memcpy(*uuid128,data+5,16);
    }


    return BT_ERR_OK;
}

err_t att_parse_read_blob_req(struct bt_pbuf_t *p,uint16_t *handle,uint16_t *offset)
{
	uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_read_blob_req data_len(%d)\n",data_len);
    *handle = bt_le_read_16(data,1);
    *offset = bt_le_read_16(data,3);

    return BT_ERR_OK;
}

err_t att_parse_read_multi_req(void)
{
	/* TODO */
	return BT_ERR_OK;
}


static err_t att_send_data(struct bt_pbuf_t *p)
{
    BT_ATT_TRACE_DEBUG("att_send_data\n");

    bt_hex_dump(p->payload,p->tot_len);

    l2cap_fixed_channel_datawrite(att_l2cap,p,L2CAP_ATT_CID);
	
	return BT_ERR_OK;
}




err_t att_mtu_req(uint16_t client_mtu)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_EXCHANGE_MTU_PACK_LEN, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_REQ_MTU;
    bt_le_store_16((uint8_t *)send_pbuf->payload,1,client_mtu);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}

err_t att_parse_mtu_rsp(struct bt_pbuf_t *p,uint16_t *server_mtu)
{
	uint8_t *data = p->payload;
    uint8_t data_len = p->len;

    BT_ATT_TRACE_DEBUG("att_parse_mtu_rsp data_len(%d)\n",data_len);
    *server_mtu = bt_le_read_16(data,1);

    return BT_ERR_OK;
}


err_t att_parse_read_type_rsp(struct bt_pbuf_t *p,uint8_t *each_len,uint8_t *data_num,uint8_t **data_list)
{
	uint8_t *data = p->payload;
    uint8_t data_len = p->len;

	*each_len = data[1];

    BT_ATT_TRACE_DEBUG("att_parse_read_type_rsp data_len(%d)\n",data_len);
	*data_num = (data_len-2)/(*each_len);
    *data_list = data+2;

    return BT_ERR_OK;
}



err_t att_parse_read_group_type_rsp(struct bt_pbuf_t *p,uint8_t *each_len,uint8_t *data_num,uint8_t **data_list,uint8_t *uuid_type)
{
	uint8_t *data = p->payload;
    uint8_t data_len = p->len;
	*each_len = data[1];

    BT_ATT_TRACE_DEBUG("att_parse_read_group_type_rsp data_len(%d)\n",data_len);
	
    *data_num = (data_len-2)/(*each_len);
	if(*each_len == 6)
		*uuid_type = ATT_UUID16_FORMAT;

	*data_list = data+2;

	return BT_ERR_OK;
}

err_t att_parse_find_type_value_rsp(struct bt_pbuf_t *p,uint8_t *info_num,uint8_t **info_list)
{
	uint8_t *data = p->payload;
    uint8_t data_len = p->len;

	*info_num = (data_len-1)/4;
	*info_list = data+1;

	return BT_ERR_OK;
}



err_t att_read_group_type_req(uint16_t start_handle,uint16_t end_handle,uint16_t uuid)
{
	/* TODO:UUIN128µÄÖ§³Ö */
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_READ_GROUP_TYPE_REQ_HDR_LEN, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_REQ_READ_BY_GRP_TYPE;
    bt_le_store_16((uint8_t *)send_pbuf->payload,1,start_handle);
	bt_le_store_16((uint8_t *)send_pbuf->payload,3,end_handle);
	bt_le_store_16((uint8_t *)send_pbuf->payload,5,uuid);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}

err_t att_find_type_value_req(uint16_t start_handle,uint16_t end_handle,uint16_t uuid,uint8_t *value,uint8_t value_len)
{
	/* TODO:UUIN128µÄÖ§³Ö */
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_FIND_TYPE_VALUE_REQ_HDR_LEN+value_len, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_REQ_FIND_TYPE_VALUE;
    bt_le_store_16((uint8_t *)send_pbuf->payload,1,start_handle);
	bt_le_store_16((uint8_t *)send_pbuf->payload,3,end_handle);
	bt_le_store_16((uint8_t *)send_pbuf->payload,5,uuid);
	memcpy((uint8_t *)send_pbuf->payload+7,value,value_len);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}

err_t att_read_type_req(uint16_t start_handle,uint16_t end_handle,uint8_t *value,uint8_t value_len)
{
	/* TODO:UUIN128µÄÖ§³Ö */
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_READ_TYPE_REQ_HDR_LEN+value_len, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_REQ_READ_BY_TYPE;
    bt_le_store_16((uint8_t *)send_pbuf->payload,1,start_handle);
	bt_le_store_16((uint8_t *)send_pbuf->payload,3,end_handle);
	memcpy((uint8_t *)send_pbuf->payload+5,value,value_len);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
}

err_t att_read_req(uint16_t handle)
{
	struct bt_pbuf_t *send_pbuf;
    if((send_pbuf = bt_pbuf_alloc(BT_PBUF_RAW, ATT_READ_REQ_PACK_LEN, BT_PBUF_RAM)) == NULL)
    {
        BT_ATT_TRACE_ERROR("ERROR:file[%s],function[%s],line[%d] bt_pbuf_alloc fail\n",__FILE__,__FUNCTION__,__LINE__);

        return BT_ERR_MEM; /* Could not allocate memory for bt_pbuf_t */
    }

    ((uint8_t *)send_pbuf->payload)[0] = ATT_REQ_READ;
    bt_le_store_16((uint8_t *)send_pbuf->payload,1,handle);

    att_send_data(send_pbuf);
    bt_pbuf_free(send_pbuf);

	return BT_ERR_OK;
	
}



