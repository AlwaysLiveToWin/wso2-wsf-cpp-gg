/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <oxs_tokens.h>

AXIS2_EXTERN axis2_char_t* AXIS2_CALL
oxs_token_get_signature_value(
    const axutil_env_t *env,
    axiom_node_t *sv_node)
{
    axis2_char_t *sv = NULL;
    /* TODO Verification */
    sv = (axis2_char_t*)oxs_axiom_get_node_content(env, sv_node);
    return sv;
}

/**
 * Creates <ds:SignatureValue> element
 */
AXIS2_EXTERN axiom_node_t* AXIS2_CALL
oxs_token_build_signature_value_element(
    const axutil_env_t *env,
    axiom_node_t *parent,
    axis2_char_t* signature_val)
{
    axiom_node_t *signature_value_node = NULL;
    axiom_element_t *signature_value_ele = NULL;
    axis2_status_t ret;
    axiom_namespace_t *ns_obj = NULL;

    ns_obj = axiom_namespace_create(env, OXS_DSIG_NS, OXS_DS);
    signature_value_ele = axiom_element_create(
        env, parent, OXS_NODE_SIGNATURE_VALUE, ns_obj, &signature_value_node);
    if(!signature_value_ele)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[rampart]Error creating signature value element.");
        axiom_namespace_free(ns_obj, env);
        return NULL;
    }

    if (signature_val)
    {
        ret  = axiom_element_set_text(signature_value_ele, env, signature_val, signature_value_node);
    }

    return signature_value_node;
}

