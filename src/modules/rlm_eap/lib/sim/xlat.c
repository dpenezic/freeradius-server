/*
 *   This program is is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or (at
 *   your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/**
 * @file rlm_eap/lib/sim/xlat.c
 * @brief EAP-SIM/EAP-AKA identity detection, creation, and decyption.
 *
 * @copyright 2017 The FreeRADIUS server project
 */

#include <freeradius-devel/radiusd.h>
#include "sim_proto.h"

static int sim_xlat_refs = 0;


/** Returns the SIM method EAP-SIM or EAP-AKA hinted at by the user identifier
 *
 *	%{sim_id_method:&id_attr}
 */
static ssize_t sim_xlat_id_method(TALLOC_CTX *ctx, char **out, UNUSED size_t outlen,
				  UNUSED void const *mod_inst, UNUSED void const *xlat_inst,
				  REQUEST *request, char const *fmt)
{
	vp_tmpl_t		*vpt;
	TALLOC_CTX		*our_ctx = talloc_init("sim_xlat");
	ssize_t			slen, len, id_len;
	char const		*p = fmt, *id, *method;
	fr_sim_id_type_t	type_hint;
	fr_sim_method_hint_t	method_hint;
	fr_dict_attr_t const	*da;

	/*
	 *  Trim whitespace
	 */
	while (isspace(*p) && p++);

	slen = tmpl_afrom_attr_substr(our_ctx, &vpt, p, REQUEST_CURRENT, PAIR_LIST_REQUEST, false, false);
	if (slen <= 0) {
		RPEDEBUG("Invalid attribute reference");
	error:
		talloc_free(our_ctx);
		return -1;
	}

	if (tmpl_aexpand(our_ctx, &id, request, vpt, NULL, NULL) < 0) {
		RPEDEBUG2("Failing expanding ID attribute");
		goto error;
	}

	id_len = talloc_array_length(id) - 1;
	len = fr_sim_id_user_len(id, id_len);
	if (len == id_len ) {
		RPEDEBUG2("SIM ID \"%pS\" is not an NAI", id);
		goto error;
	}

	if (fr_sim_id_type(&type_hint, &method_hint, id, len) < 0) {
		RPEDEBUG2("SIM ID \"%pS\" has unrecognised format", id);
		goto error;
	}

	da = fr_dict_attr_by_num(NULL, 0, FR_SIM_METHOD_HINT);
	if (!da) {
		REDEBUG("Missing Sim-Method-Hint attribute");
		goto error;
	}

	method = fr_dict_enum_alias_by_value(NULL, da, fr_box_uint32(method_hint));
	if (!method) {
		REDEBUG("Missing Sim-Method-Hint value");
		goto error;
	}
	*out = talloc_typed_strdup(ctx, method);
	talloc_free(our_ctx);

	return talloc_array_length(*out) - 1;
}

/** Returns the type of identity used
 *
 *	%{sim_id_type:&id_attr}
 */
static ssize_t sim_xlat_id_type(TALLOC_CTX *ctx, char **out, UNUSED size_t outlen,
				UNUSED void const *mod_inst, UNUSED void const *xlat_inst,
				REQUEST *request, char const *fmt)
{
	vp_tmpl_t		*vpt;
	TALLOC_CTX		*our_ctx = talloc_init("sim_xlat");
	ssize_t			slen, user_len, id_len;
	char const		*p = fmt, *id, *method;
	fr_sim_id_type_t	type_hint;
	fr_sim_method_hint_t	method_hint;
	fr_dict_attr_t const	*da;

	/*
	 *  Trim whitespace
	 */
	while (isspace(*p) && p++);

	slen = tmpl_afrom_attr_substr(our_ctx, &vpt, p, REQUEST_CURRENT, PAIR_LIST_REQUEST, false, false);
	if (slen <= 0) {
		RPEDEBUG("Invalid attribute reference");
	error:
		talloc_free(our_ctx);
		return -1;
	}

	if (tmpl_aexpand(our_ctx, &id, request, vpt, NULL, NULL) < 0) {
		RPEDEBUG2("Failing expanding ID attribute");
		goto error;
	}

	id_len = talloc_array_length(id) - 1;
	user_len = fr_sim_id_user_len(id, id_len);
	if (user_len == id_len ) {
		RPEDEBUG2("SIM ID \"%pS\" is not an NAI", id);
		goto error;
	}

	if (fr_sim_id_type(&type_hint, &method_hint, id, user_len) < 0) {
		RPEDEBUG2("SIM ID \"%pS\" has unrecognised format", id);
		goto error;
	}

	da = fr_dict_attr_by_num(NULL, 0, FR_SIM_IDENTITY_TYPE);
	if (!da) {
		REDEBUG("Missing Sim-Method-Hint attribute");
		goto error;
	}

	method = fr_dict_enum_alias_by_value(NULL, da, fr_box_uint32(type_hint));
	if (!method) {
		REDEBUG("Missing Sim-Method-Hint value");
		goto error;
	}
	*out = talloc_typed_strdup(ctx, method);
	talloc_free(our_ctx);

	return talloc_array_length(*out) - 1;
}

/** Returns the key index from a 3gpp pseudonym
 *
 *	%{sim_id_3gpp_pseudonym_key_index:&id_attr}
 *
 */
static ssize_t sim_xlat_3gpp_pseudonym_key_index(TALLOC_CTX *ctx, char **out, UNUSED size_t outlen,
						 UNUSED void const *mod_inst, UNUSED void const *xlat_inst,
						 REQUEST *request, char const *fmt)
{
	vp_tmpl_t	*vpt;
	TALLOC_CTX	*our_ctx = talloc_init("sim_xlat");
	ssize_t		slen, user_len, id_len;
	char const	*p = fmt, *id;

	/*
	 *  Trim whitespace
	 */
	while (isspace(*p) && p++);

	slen = tmpl_afrom_attr_substr(our_ctx, &vpt, p, REQUEST_CURRENT, PAIR_LIST_REQUEST, false, false);
	if (slen <= 0) {
		RPEDEBUG("Invalid attribute reference");
	error:
		talloc_free(our_ctx);
		return -1;
	}

	if (tmpl_aexpand(our_ctx, &id, request, vpt, NULL, NULL) < 0) {
		RPEDEBUG2("Failing expanding ID attribute");
		goto error;
	}

	id_len = talloc_array_length(id) - 1;
	user_len = fr_sim_id_user_len(id, id_len);
	if (user_len != SIM_3GPP_PSEUDONYM_LEN) {
		REDEBUG2("3gpp pseudonym incorrect length, expected %i bytes, got %zu bytes",
			 SIM_3GPP_PSEUDONYM_LEN, user_len);
		goto error;
	}

	MEM(*out = talloc_typed_asprintf(ctx, "%i", fr_sim_id_3gpp_pseudonym_tag(id)));
	talloc_free(our_ctx);

	return talloc_array_length(*out) - 1;
}

/** Decrypts a 3gpp pseudonym
 *
 *	%{sim_id_3gpp_pseudonym_decrypt_nai:&id_attr &key_attr}
 *
 */
static ssize_t sim_xlat_3gpp_pseudonym_decrypt_nai(TALLOC_CTX *ctx, char **out, UNUSED size_t outlen,
						   UNUSED void const *mod_inst, UNUSED void const *xlat_inst,
						   REQUEST *request, char const *fmt)
{
	vp_tmpl_t	*id_vpt, *key_vpt;
	TALLOC_CTX	*our_ctx = talloc_init("sim_xlat");
	ssize_t		slen, user_len, id_len, key_len;
	uint8_t		tag;
	char		out_tag;
	uint8_t		*key;
	char		decrypted[SIM_IMSI_MAX_LEN + 1];
	char const	*p = fmt, *id;

	/*
	 *  Trim whitespace
	 */
	while (isspace(*p) && p++);

	slen = tmpl_afrom_attr_substr(our_ctx, &id_vpt, p, REQUEST_CURRENT, PAIR_LIST_REQUEST, false, false);
	if (slen <= 0) {
		RPEDEBUG("Invalid ID attribute reference");
	error:
		talloc_free(our_ctx);
		return -1;
	}

	p += slen;
	if (*p != ' ') {
		REDEBUG2("Missing key argument");
		goto error;
	}
	p++;

	slen = tmpl_afrom_attr_substr(our_ctx, &key_vpt, p, REQUEST_CURRENT, PAIR_LIST_REQUEST, false, false);
	if (slen <= 0) {
		RPEDEBUG("Invalid key attribute reference");
		goto error;
	}

	if (tmpl_aexpand(our_ctx, &id, request, id_vpt, NULL, NULL) < 0) {
		RPEDEBUG2("Failing expanding ID attribute");
		goto error;
	}


	if (tmpl_aexpand(our_ctx, &key, request, key_vpt, NULL, NULL) < 0) {
		RPEDEBUG2("Failing expanding Key attribute");
		goto error;
	}

	id_len = talloc_array_length(id);
	user_len = fr_sim_id_user_len(id, id_len);
	if (user_len != SIM_3GPP_PSEUDONYM_LEN) {
		REDEBUG2("3gpp pseudonym incorrect length, expected %i bytes, got %zu bytes",
			 SIM_3GPP_PSEUDONYM_LEN, user_len);
		return -1;
	}

	key_len = talloc_array_length(key);
	if (key_len != 16) {
		REDEBUG2("Decryption key incorrect length, expected %i bytes, got %zu bytes", 16, key_len);
		return -1;
	}

	tag = fr_sim_id_3gpp_pseudonym_tag(id);
	switch (tag) {
	case 59:		/* 7 in the base64 alphabet (SIM) */
		out_tag = SIM_ID_TAG_PERMANENT_SIM;
		break;

	case 58:		/* 6 in the base64 alphabet (AKA) */
		out_tag = SIM_ID_TAG_PERMANENT_AKA;
		break;

	default:
		REDEBUG2("Unexpected tag value (%u) in SIM ID \"%pS\"", tag, id);
		return -1;
	}

	RDEBUG2("Decrypting \"%.*s\"", (int)user_len, id);
	if (fr_sim_id_3gpp_pseudonym_decrypt(decrypted, id, key) < 0) {
		RPEDEBUG2("Failed decrypting SIM ID");
		return -1;
	}

	/*
	 *	Recombine unencrypted IMSI with @domain
	 */
	MEM(*out = talloc_typed_asprintf(ctx, "%c%s%s", out_tag, decrypted, id + user_len));
	talloc_free(our_ctx);

	return talloc_array_length(*out) - 1;
}

/** Decrypts a 3gpp pseudonym
 *
 *	%{sim_id_3gpp_pseudonym_encrypt:&id_attr &key_attr key_index}
 *
 */
static ssize_t sim_xlat_3gpp_pseudonym_encrypt_nai(TALLOC_CTX *ctx, char **out, UNUSED size_t outlen,
						   UNUSED void const *mod_inst, UNUSED void const *xlat_inst,
						   REQUEST *request, char const *fmt)
{
	vp_tmpl_t		*id_vpt, *key_vpt;
	TALLOC_CTX		*our_ctx = talloc_init("sim_xlat");
	ssize_t			slen, user_len, id_len, key_len;
	uint8_t			*key, tag = 0;
	unsigned long		key_index;
	char			encrypted[SIM_3GPP_PSEUDONYM_LEN + 1];
	char const		*p = fmt, *id;
	fr_sim_id_type_t	type_hint;
	fr_sim_method_hint_t	method_hint;

	/*
	 *  Trim whitespace
	 */
	while (isspace(*p) && p++);

	slen = tmpl_afrom_attr_substr(our_ctx, &id_vpt, p, REQUEST_CURRENT, PAIR_LIST_REQUEST, false, false);
	if (slen <= 0) {
		RPEDEBUG("Invalid ID attribute reference");
	error:
		talloc_free(our_ctx);
		return -1;
	}

	p += slen;
	if (*p != ' ') {
		REDEBUG2("Missing key argument");
		goto error;
	}
	p++;

	slen = tmpl_afrom_attr_substr(our_ctx, &key_vpt, p, REQUEST_CURRENT, PAIR_LIST_REQUEST, false, false);
	if (slen <= 0) {
		RPEDEBUG("Invalid key attribute reference");
		goto error;
	}
	p += slen;

	if (*p != ' ') {
		REDEBUG2("Missing key index");
		goto error;
	}
	p++;

	/*
	 *	Get the key index
	 */
	key_index = strtoul(p, NULL, 10);
	if (key_index > 15) {
		REDEBUG2("Key index must be between 0-15");
		goto error;
	}

	/*
	 *	Get the ID
	 */
	if (tmpl_aexpand(our_ctx, &id, request, id_vpt, NULL, NULL) < 0) {
		RPEDEBUG2("Failing expanding ID attribute");
		goto error;
	}

	id_len = talloc_array_length(id) - 1;
	user_len = fr_sim_id_user_len(id, id_len);
	if (user_len > (SIM_IMSI_MAX_LEN + 1)) {	/* +1 for tag */
		REDEBUG2("3gpp pseudonym incorrect length, expected less than %i bytes, got %zu bytes",
			 SIM_IMSI_MAX_LEN + 1, user_len);
		return -1;
	}

	/*
	 *	Get the key
	 */
	if (tmpl_aexpand(our_ctx, &key, request, key_vpt, NULL, NULL) < 0) {
		RPEDEBUG2("Failing expanding Key attribute");
		goto error;
	}

	key_len = talloc_array_length(key);
	if (key_len != 16) {
		REDEBUG2("Encryption key incorrect length, expected %i bytes, got %zu bytes", 16, key_len);
		return -1;
	}

	/*
	 *	Determine what type/method hints are in
	 *	the current ID.
	 */
	if (fr_sim_id_type(&type_hint, &method_hint, id, user_len) < 0) {
		RPEDEBUG2("SIM ID \"%pS\" has unrecognised format", id);
		goto error;
	}

	if (type_hint != SIM_ID_TYPE_PERMANENT) {
		REDEBUG2("SIM ID \"%pS\" is not a permanent identity (IMSI)", id);
		goto error;
	}

	switch (method_hint) {
	case SIM_METHOD_HINT_SIM:
		tag = 59;	/* 7 in the base64 alphabet */
		break;

	case SIM_METHOD_HINT_AKA:
		tag = 58;	/* 6 in the base64 alphabet */
		break;

	case SIM_METHOD_HINT_UNKNOWN:
		REDEBUG2("SIM ID \"%pS\" does not contain a method hint", id);
		goto error;
	}

	/*
	 *	Encrypt the IMSI
	 *
	 *	Strip existing tag from the permanent id
	 */
	if (fr_sim_id_3gpp_pseudonym_encrypt(encrypted, id + 1, user_len - 1, tag, (uint8_t)key_index, key) < 0) {
		RPEDEBUG2("Failed encrypting SIM ID \"%pS\"", id);
		return -1;
	}

	/*
	 *	Recombine encrypted IMSI with @domain
	 */
	MEM(*out = talloc_typed_asprintf(ctx, "%s%s", encrypted, id + user_len));
	talloc_free(our_ctx);

	return talloc_array_length(*out) - 1;
}

void sim_xlat_register(void)
{
	if (sim_xlat_refs) {
		sim_xlat_refs++;
		return;
	}

	xlat_register(NULL, "sim_id_method", sim_xlat_id_method, NULL, NULL, 0, 0, true);
	xlat_register(NULL, "sim_id_type", sim_xlat_id_type, NULL, NULL, 0, 0, true);
	xlat_register(NULL, "3gpp_pseudonym_key_index",
		      sim_xlat_3gpp_pseudonym_key_index, NULL, NULL, 0, 0, true);
	xlat_register(NULL, "3gpp_pseudonym_decrypt_nai",
		      sim_xlat_3gpp_pseudonym_decrypt_nai, NULL, NULL, 0, 0, true);
	xlat_register(NULL, "3gpp_pseudonym_encrypt_nai",
		      sim_xlat_3gpp_pseudonym_encrypt_nai, NULL, NULL, 0, 0, true);
	sim_xlat_refs = 1;
}

void sim_xlat_unregister(void)
{
	if (sim_xlat_refs > 1) {
		sim_xlat_refs--;
		return;
	}

	xlat_unregister("sim_id_method");
	xlat_unregister("sim_id_type");
	xlat_unregister("3gpp_pseudonym_key_index");
	xlat_unregister("3gpp_pseudonym_decrypt_nai");
	xlat_unregister("3gpp_pseudonym_encrypt_nai");
	sim_xlat_refs = 0;
}
