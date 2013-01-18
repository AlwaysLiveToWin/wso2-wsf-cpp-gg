/*
 * Copyright 2005-2009 WSO2, Inc. http://wso2.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <WSFError.h>
#include <sstream>
#include <Environment.h>
#include <axis2_callback.h>
#include <axis2_conf_init.h>
#include <ServiceClient.h>
#include <assert.h>

using namespace std;
using namespace wso2wsf;


void ServiceClient::resetAxiomPayloadNamespace(OMElement *payload)
{
	if(payload)
	{
		if (payload->getNamespaceLocal(false))
		{
			(payload->getNamespaceLocal(false))->setAxiomNamespace(NULL);
		}
		payload->setAxiomNode(NULL);
	}
}

/** @brief callbackOnComplete
  *
  * onComplete function that is used in a non-blocking scenario.
  */
axis2_status_t AXIS2_CALL callbackOnComplete(axis2_callback_t * callback, const axutil_env_t * env)
{
    if (!callback)
    {
        return AXIS2_FAILURE;
    }
    axiom_soap_envelope_t * soap_envelope = NULL;
    axiom_node_t * ret_node = NULL;
	ICallback *_m_callback = (ICallback*)axis2_callback_get_data(callback);
	soap_envelope = axis2_callback_get_envelope(callback, env);
    if (soap_envelope)
    {
        ret_node = axiom_soap_envelope_get_base_node(soap_envelope, env);
        if (ret_node && (axiom_node_get_node_type(ret_node, env) == AXIOM_ELEMENT))
        {
            _m_callback->onComplete(new OMElement(NULL, ret_node));
            return AXIS2_SUCCESS;
        }
    }
    _m_callback->onComplete(new OMElement("Empty"));
    return AXIS2_FAILURE;
}

/** @brief callbackOnFault
  *
  * onFault function that is used in a non-blocking scenario.
  */
axis2_status_t AXIS2_CALL callbackOnFault(axis2_callback_t * callback, const axutil_env_t * env, int exception)
{
	ICallback *_m_callback = (ICallback*)axis2_callback_get_data(callback);
    if (!_m_callback)
    {
        return AXIS2_FAILURE;
    }
	OMElement *faultEle = new OMElement("Fault");
    OMElement * faultcode = new OMElement(faultEle, "faultcode");
    stringstream out;
    out << exception;
    faultcode->setText(out.str());
    OMElement * faultstring = new OMElement(faultEle, "faultstring");
    faultstring->setText(AXIS2_ERROR_GET_MESSAGE(env->error));
    OMElement * faultactor = new OMElement(faultEle, "faultactor");
    OMElement * details = new OMElement(faultEle, "details");
    details->setText("Fault generated by WSF/C++");
    _m_callback->onFault(faultEle);
    return AXIS2_SUCCESS;
}

axis2_svc_client_t* WSF_CALL ServiceClient::getAxis2SvcClient()
{
	assert(isValid);
	return _wsf_service_client;
}

/** @brief ~ServiceClient
  *
  * @todo: document this function
  */
ServiceClient::~ServiceClient()
{
    if (_last_response_soap_envelope_element)
    {
        _last_response_soap_envelope_element->setAxiomNode(NULL);
        delete _last_response_soap_envelope_element;
    }
    if (_last_soap_fault)
    {
        _last_soap_fault->setAxiomNode(NULL);
        delete _last_soap_fault;
    }
    if (_policy)
    {
        _policy->setNeethiPolicy(NULL);
        delete _policy;
    }
    if (_options)
    {
        _options->_wsf_options = NULL;
        delete _options;
    }
	/** Freeing conf context is not done since it is freed by service client free */    
    axis2_svc_client_finalize_invoke(_wsf_service_client, Environment::getEnv());
    axis2_svc_client_free(_wsf_service_client, Environment::getEnv());

	_conf_ctx = NULL;
}

/** @brief ServiceClient
  *
  * @todo: document this function
  */
 ServiceClient::ServiceClient(const axis2_conf_ctx_t * conf_ctx, std::string endpoint_address)
{
	isValid =false;
    _conf_ctx = conf_ctx;
    _endpoint_address = endpoint_address;
    _repo_home = "";
    _wsf_service_client = NULL;
    _last_soap_fault = NULL;
    _last_response_soap_envelope_element = NULL;
    _last_response_soap_envelope = NULL;
    _options = NULL;
    _policy = NULL;
	
    isValid = initializeClient();
}

/** @brief ServiceClient
  *
  * @todo: document this function
  */
 ServiceClient::ServiceClient(std::string repo_home, std::string endpoint_address)
{
	isValid = false;
    _conf_ctx = NULL;
    _endpoint_address = endpoint_address;
    _repo_home = repo_home;
    _wsf_service_client = NULL;
    _last_soap_fault = NULL;
    _last_response_soap_envelope_element = NULL;
    _last_response_soap_envelope = NULL;
    _options = NULL;
    _policy = NULL;

    isValid = initializeClient();
}

/** @brief ServiceClient
  *
  * @todo: document this function
  */
 ServiceClient::ServiceClient(std::string endpoint_address)
{
	isValid = false;
    _conf_ctx = NULL;
    _endpoint_address = endpoint_address;
	if(AXIS2_GETENV("WSFCPP_HOME"))
	{
		_repo_home = string(AXIS2_GETENV("WSFCPP_HOME"));
	}else
	{
		std::cout<<"Warning WSFCPP_HOME not defined"<<std::endl;
		_repo_home = string(".");
	}
    _wsf_service_client = NULL;
    _last_soap_fault = NULL;
    _last_response_soap_envelope_element = NULL;
    _last_response_soap_envelope = NULL;
    _options = NULL;
    _policy = NULL;

    isValid = initializeClient();
}

/** @brief InitializeClient
  *
  * @todo: document this function
  */
bool ServiceClient::initializeClient()
{
	if (_conf_ctx)
	{
		if (_repo_home == "")
		{
			axis2_conf_t * conf = axis2_conf_ctx_get_conf(_conf_ctx, Environment::getEnv());
			_repo_home = axis2_conf_get_repo(conf, Environment::getEnv());
		}
		_wsf_service_client = axis2_svc_client_create_with_conf_ctx_and_svc(Environment::getEnv(), _repo_home.c_str(),
			const_cast<axis2_conf_ctx_t *>(_conf_ctx), NULL);
	}
    else
    {
	    _wsf_service_client = axis2_svc_client_create(Environment::getEnv(), _repo_home.c_str());
    }

	if (!_wsf_service_client)
	{
		WSF_LOG_CRITICAL_MSG(Environment::getEnv()->log, WSF_LOG_SI, "Cannot create service client");
		return false;
	}

	_options = new Options();
	if (!_options->_wsf_options)
	{
		delete _options;
		return false;
	}
	else
	{
		_options->setXMLParserReset(true);
		_options->setUseSeparateListener(false);
		_options->setTo(_endpoint_address);
		axis2_status_t status = axis2_svc_client_set_options(
            _wsf_service_client, Environment::getEnv(), _options->_wsf_options);
		if (status != AXIS2_SUCCESS)
		{
			return false;
		}
	}
    return true;
}

/** @brief setOptions
  *
  * @todo: document this function
  */
bool ServiceClient::setOptions(Options * options)
{
	assert(isValid);
    axis2_status_t status = AXIS2_FAILURE;
    if (options != NULL)
    {
        if (_options->_wsf_options == options->_wsf_options)
        {
            return true;
        }
        _options = options;
        status = axis2_svc_client_set_options(_wsf_service_client, Environment::getEnv(), _options->_wsf_options);
    }
    return (status == AXIS2_SUCCESS);
}

/** @brief getLastResponseSoapEnvelopeString
  *
  * @todo: document this function
  */
string ServiceClient::getLastResponseSoapEnvelopeString()
{
	assert(isValid);
    axiom_soap_envelope_t * envelope =
        axis2_svc_client_get_last_response_soap_envelope(_wsf_service_client, Environment::getEnv());
    if (!envelope)
    {
        return "";
    }
    _last_response_soap_envelope = envelope;
    axiom_node_t * node = axiom_soap_envelope_get_base_node(_last_response_soap_envelope, Environment::getEnv());
    axis2_char_t *envstr = axiom_node_to_string(node, Environment::getEnv());
	if(envstr)
		return envstr;
	return "";
}

/** @brief getLastResponseSoapEnvelope
  *
  * @todo: document this function
  */
OMElement * ServiceClient::getLastResponseSoapEnvelope()
{
	assert(isValid);
    axiom_soap_envelope_t * envelope =
        axis2_svc_client_get_last_response_soap_envelope(_wsf_service_client, Environment::getEnv());
    if (!envelope)
    {
        return NULL;
    }
    _last_response_soap_envelope = envelope;
    axiom_node_t * node = axiom_soap_envelope_get_base_node(_last_response_soap_envelope, Environment::getEnv());
    axiom_element_t * element = NULL;
    if (node && (axiom_node_get_node_type(node, Environment::getEnv()) == AXIOM_ELEMENT))
    {
        element = (axiom_element_t *)axiom_node_get_data_element(node, Environment::getEnv());
    }
    if (!element)
    {
        return NULL;
    }
    axis2_bool_t has_fault =
        axis2_svc_client_get_last_response_has_fault(_wsf_service_client, Environment::getEnv());
    if (has_fault)
    {
        if (_last_soap_fault)
        {
            _last_soap_fault->setAxiomNode(NULL);
            delete _last_soap_fault;
        }
        _last_soap_fault = new OMElement(NULL, node);
        return NULL;
    }
    if (_last_response_soap_envelope_element)
    {
        _last_response_soap_envelope_element->setAxiomNode(NULL);
        delete _last_response_soap_envelope_element;
    }
    _last_response_soap_envelope_element = new OMElement(NULL, node);
    return _last_response_soap_envelope_element;
}

/** @brief request
  *
  * @todo: document this function
  */
OMElement * ServiceClient::request(OMElement * payload, std::string operation, std::string action) throw (WSFault)
{
    return request(payload, NULL, operation, action);
}

/** @brief request
  *
  * @todo: document this function
  */
OMElement * ServiceClient::request(OMElement * payload, ICallback * callback, std::string action) throw (WSFault)
{
    return request(payload, callback, "", action);
}

/** @brief request
  *
  * @todo: document this function
  */
OMElement * ServiceClient::request(OMElement * payload, ICallback * callback, std::string operation, std::string action) throw (WSFault)
{
	assert(isValid);
    axis2_status_t status = AXIS2_FAILURE;
	axiom_node_t *payload_axiom_node = NULL;
	if(payload)
	{
		payload_axiom_node = payload->getAxiomNode();
	}
   
    if (action != "")
    {
        if (_options->getSoapVersion() == AXIOM_SOAP11)
        {
            axutil_string_t * soap_action = axutil_string_create(Environment::getEnv(), action.c_str());
			/** cannot fail */
            status = axis2_options_set_soap_action(_options->_wsf_options, Environment::getEnv(), soap_action);
            axutil_string_free(soap_action, Environment::getEnv());
        }
        else
        {
            axutil_qname_t * qname = axutil_qname_create(Environment::getEnv(), AXIS2_MODULE_ADDRESSING, NULL, NULL);
            axis2_bool_t engaged =
                axis2_svc_is_module_engaged(axis2_svc_client_get_svc(_wsf_service_client, Environment::getEnv()),
                Environment::getEnv(), qname);
            axutil_qname_free(qname, Environment::getEnv());
            if (engaged)
            {
				/** cannot fail */
                status = axis2_options_set_action(_options->_wsf_options, Environment::getEnv(), action.c_str());
                
            }
        }
    }
    axiom_node_t * node;
    if (!callback)
    {
        if (operation != "")
        {
            axutil_qname_t * qname =  axutil_qname_create(Environment::getEnv(), operation.c_str(), NULL, NULL);
            node = axis2_svc_client_send_receive_with_op_qname(_wsf_service_client, Environment::getEnv(),
                qname, payload_axiom_node);
            axutil_qname_free(qname, Environment::getEnv());
        }
        else
        {
            node = axis2_svc_client_send_receive(_wsf_service_client, Environment::getEnv(), payload_axiom_node);
        }
        if (!node)
        {
			resetAxiomPayloadNamespace(payload);
			throw WSFault(SEND_RECIEVE_OPERATION_FAILED);
        }
        else
        {
            axiom_element_t * element = NULL;
            if (node && (axiom_node_get_node_type(node, Environment::getEnv()) == AXIOM_ELEMENT))
            {
                element = (axiom_element_t *)axiom_node_get_data_element(node, Environment::getEnv());
            }
            if (!element)
            {
               resetAxiomPayloadNamespace(payload);
                throw WSFault(UNABLE_TO_RETRIEVE_RESPONSE_ELEMENT);
            }
            axis2_bool_t has_fault =
                axis2_svc_client_get_last_response_has_fault(_wsf_service_client, Environment::getEnv());
            if (has_fault)
            {
                if (_last_soap_fault)
                {
                    _last_soap_fault->setAxiomNode(NULL);
                    delete _last_soap_fault;
                }
                _last_soap_fault = new OMElement(NULL, node);
                resetAxiomPayloadNamespace(payload);
                throw WSFault(SOAP_FAULT_RECIEVED);
            }
            if (_last_response_soap_envelope_element)
            {
                _last_response_soap_envelope_element->setAxiomNode(NULL);
                delete _last_response_soap_envelope_element;
            }
            _last_response_soap_envelope_element = new OMElement(NULL, node);
			resetAxiomPayloadNamespace(payload);
            return _last_response_soap_envelope_element;
        }
    }
    else
    {
        axis2_callback_t * callback_c = axis2_callback_create(Environment::getEnv());
        if (!callback_c)
        {
            if (payload->getNamespace())
            {
                (payload->getNamespace())->setAxiomNamespace(NULL);
            }
            payload->setAxiomNode(NULL);
            throw WSFault(CREATION_OF_CALLBACK_OBJECT_FAILED);
        }
        axis2_callback_set_on_complete(callback_c, callbackOnComplete);
        axis2_callback_set_on_error(callback_c, callbackOnFault);
		axis2_callback_set_data(callback_c, (void*)callback);

        if (operation != "")
        {
            axutil_qname_t * qname =  axutil_qname_create(Environment::getEnv(), operation.c_str(), NULL, NULL);
            axis2_svc_client_send_receive_non_blocking_with_op_qname(_wsf_service_client, Environment::getEnv(),
                qname, payload_axiom_node, callback_c);
            axutil_qname_free(qname, Environment::getEnv());
        }
        else
        {
            axis2_svc_client_send_receive_non_blocking(_wsf_service_client, Environment::getEnv(), payload_axiom_node, callback_c);
        }
        resetAxiomPayloadNamespace(payload);
		return NULL;
    }
}

/** @brief request
  *
  * @todo: document this function
  */
OMElement * ServiceClient::request(OMElement * payload, std::string action) throw (WSFault)
{
    return request(payload, NULL, "", action);
}

/** @brief send
  *
  * @todo: document this function
  */
bool ServiceClient::send(OMElement * payload, bool robust, std::string action) throw (WSFault)
{
    return send(payload, robust, "", action);
}

/** @brief send
  *
  * @todo: document this function
  */
bool ServiceClient::send(OMElement * payload, bool robust, std::string operation, std::string action) throw (WSFault)
{
	assert(isValid);
    axis2_status_t status = AXIS2_FAILURE;
	axiom_node_t *payload_axiom_node = NULL;
    if (payload)
    {
		payload_axiom_node = payload->getAxiomNode();
	}
    if (action != "")
    {
        if (_options->getSoapVersion() == AXIOM_SOAP11)
        {
            axutil_string_t * soap_action = axutil_string_create(Environment::getEnv(), action.c_str());
            status = axis2_options_set_soap_action(_options->_wsf_options, Environment::getEnv(), soap_action);
            axutil_string_free(soap_action, Environment::getEnv());
        }
        else
        {
            axutil_qname_t * qname = axutil_qname_create(Environment::getEnv(), AXIS2_MODULE_ADDRESSING, NULL, NULL);
            axis2_bool_t engaged =
                axis2_svc_is_module_engaged(axis2_svc_client_get_svc(_wsf_service_client, Environment::getEnv()),
                Environment::getEnv(), qname);
            axutil_qname_free(qname, Environment::getEnv());
            if (engaged)
            {
                status = axis2_options_set_action(_options->_wsf_options, Environment::getEnv(), action.c_str());
            }
        }
    }
    if (robust)
    {
        if (operation != "")
        {
            axutil_qname_t * qname =  axutil_qname_create(Environment::getEnv(), operation.c_str(), NULL, NULL);
            status = axis2_svc_client_send_robust_with_op_qname(_wsf_service_client, Environment::getEnv(),
                qname, payload_axiom_node);
            axutil_qname_free(qname, Environment::getEnv());
        }
        else
        {
            status = axis2_svc_client_send_robust(_wsf_service_client, Environment::getEnv(), payload_axiom_node);
        }
        if (status != AXIS2_SUCCESS)
        {
            resetAxiomPayloadNamespace(payload);
			throw WSFault(ROBUST_SEND_OPERATION_FAILED);
        }
        axis2_bool_t has_fault = axis2_svc_client_get_last_response_has_fault(_wsf_service_client, Environment::getEnv());
        if (has_fault)
        {
            resetAxiomPayloadNamespace(payload);
			throw WSFault(SOAP_FAULT_RECIEVED);
        }
    }
    else
    {
        if (operation != "")
        {
            axutil_qname_t * qname =  axutil_qname_create(Environment::getEnv(), operation.c_str(), NULL, NULL);
            axis2_svc_client_fire_and_forget_with_op_qname(_wsf_service_client, Environment::getEnv(),
                qname, payload_axiom_node);
            axutil_qname_free(qname, Environment::getEnv());
        }
        else
        {
            axis2_svc_client_fire_and_forget(_wsf_service_client, Environment::getEnv(), payload_axiom_node);
        }
    }
    resetAxiomPayloadNamespace(payload);
	return true;
}

/** @brief send
  *
  * @todo: document this function
  */
bool ServiceClient::send(OMElement * payload, std::string operation, std::string action) throw (WSFault)
{
    return send(payload, true, operation, action);
}

/** @brief send
  *
  * @todo: document this function
  */
bool ServiceClient::send(OMElement * payload, std::string action) throw (WSFault)
{
    return send(payload, true, "", action);
}

/** @brief setPolicy
  *
  * @todo: document this function
  */
bool ServiceClient::setPolicy(NeethiPolicy * policy)
{
	assert(isValid);
    if (!policy)
    {
        return false;
    }
    else if (_policy)
    {
        _policy->setNeethiPolicy(NULL);
        delete _policy;
    }
    _policy = policy;
    axis2_status_t status =
        axis2_svc_client_set_policy(_wsf_service_client, Environment::getEnv(), policy->getNeethiPolicy());
    return (status == AXIS2_SUCCESS);
}

/** @brief removeAllHeaders
  *
  * @todo: document this function
  */
void ServiceClient::removeAllHeaders()
{
	assert(isValid);
    axis2_svc_client_remove_all_headers(_wsf_service_client, Environment::getEnv());
}

/** @brief addHeader
  *
  * @todo: document this function
  */
bool ServiceClient::addHeader(OMElement * header)
{
	assert(isValid);
    axis2_status_t status =
        axis2_svc_client_add_header(_wsf_service_client, Environment::getEnv(), header->getAxiomNode());
    return (status == AXIS2_SUCCESS);
}

/** @brief disengageModule
  *
  * @todo: document this function
  */
bool ServiceClient::disengageModule(std::string module_name)
{
	assert(isValid);
    axis2_status_t status =
        axis2_svc_client_disengage_module(_wsf_service_client, Environment::getEnv(), module_name.c_str());
    return (status == AXIS2_SUCCESS);
}

/** @brief engageModule
  *
  * @todo: document this function
  */
bool ServiceClient::engageModule(std::string module_name)
{
	assert(isValid);
    axis2_status_t status =
        axis2_svc_client_engage_module(_wsf_service_client, Environment::getEnv(), module_name.c_str());
    return (status == AXIS2_SUCCESS);
}

/** @brief getOptions
  *
  * @todo: document this function
  */
Options * ServiceClient::getOptions()
{
	assert(isValid);
    return _options;
}

/** @brief getLastSOAPFault
  *
  * @todo: document this function
  */
OMElement * ServiceClient::getLastSOAPFault()
{
	assert(isValid);
    getLastResponseSoapEnvelope();
    return _last_soap_fault;
}

axis2_conf_ctx_t* ServiceClient::getAxis2ConfCtx(std::string repositoryPath)
{
	assert(isValid);
	axis2_conf_ctx_t* conf_ctx = NULL;
	conf_ctx = axis2_build_conf_ctx(Environment::getEnv(), repositoryPath.c_str());
	return conf_ctx;
}

bool ServiceClient::getProxyAuthRequired()
{
	assert(isValid);
	axis2_bool_t status  = AXIS2_FALSE;
	status = axis2_svc_client_get_proxy_auth_required(_wsf_service_client, Environment::getEnv());
	return status ? true : false;
}

bool ServiceClient::setProxy(string proxyHost, string proxyPort)
{
	assert(isValid);
	axis2_status_t status = AXIS2_SUCCESS;
	status = axis2_svc_client_set_proxy(_wsf_service_client, Environment::getEnv(), 
		(axis2_char_t *)proxyHost.c_str(), (axis2_char_t *)proxyPort.c_str());
	return status ? true : false;
}

bool ServiceClient::setProxyWithAuth(string proxyHost, string proxyPort, string username, string password)
{
	assert(isValid);
	axis2_status_t status = AXIS2_SUCCESS;
	status = axis2_svc_client_set_proxy_with_auth(_wsf_service_client, Environment::getEnv(), 
		(axis2_char_t *)proxyHost.c_str(), (axis2_char_t *)proxyPort.c_str(), (axis2_char_t *)username.c_str(), 
		(axis2_char_t *)password.c_str());
	return status ? true: false;
}

bool ServiceClient::isValidClient()
{
	return isValid;
}

MessageContext* WSF_CALL ServiceClient::getMessageContext(axis2_wsdl_msg_labels_t message_lebel)
{
	if(_wsf_service_client)
	{
		axis2_op_client_t *op_client = NULL;
		op_client = axis2_svc_client_get_op_client(_wsf_service_client, Environment::getEnv());
		if(op_client)
		{
			const axis2_msg_ctx_t* msg_ctx = NULL;
			msg_ctx = axis2_op_client_get_msg_ctx(op_client, Environment::getEnv(), message_lebel);
			if(msg_ctx)
				return new MessageContext((axis2_msg_ctx_t*)msg_ctx);
			else
				return NULL;
		}
	}
	return NULL;
}


MessageContext* WSF_CALL ServiceClient::getInMessageContext()
{
	return getMessageContext(AXIS2_WSDL_MESSAGE_LABEL_IN);
}
MessageContext* WSF_CALL ServiceClient::getOutMessageContext()
{
	return getMessageContext(AXIS2_WSDL_MESSAGE_LABEL_OUT);
}
    
bool WSF_CALL ServiceClient::close()
{
    axis2_status_t status = AXIS2_FAILURE;

    status = axis2_svc_client_close(_wsf_service_client, Environment::getEnv());
	
    return status ? true: false;
}
