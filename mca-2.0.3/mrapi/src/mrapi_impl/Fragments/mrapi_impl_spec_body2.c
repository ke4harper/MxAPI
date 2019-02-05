  /***************************************************************************
  Function:mrapi_impl_valid_parameters_param

  Description: checks that the parameter is valid
  
  Parameters: 

  Returns:  boolean indicating success or failure
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_valid_parameters_param (mrapi_parameters_t mrapi_parameters)
  {
    return MRAPI_TRUE;
  }

  /***************************************************************************
  Function:mrapi_impl_valid_info_param

  Description: checks that the parameter is valid

  Parameters:

  Returns:  boolean indicating success or failure
  ***************************************************************************/
  mrapi_boolean_t mrapi_impl_valid_info_param (const mrapi_info_t* mrapi_info)
  {
    return (mrapi_info != NULL);
  }
