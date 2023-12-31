//===----------------------------------------------------------------------===//
//
// fmgroids.h
//    Macros that define the OIDs of built-in functions.
//
// These macros can be used to avoid a catalog lookup when a specific
// fmgr-callable function needs to be referenced.
//
// Portions Copyright (c) 1996-2001, PostgreSQL Global Development Group
// Portions Copyright (c) 1994, Regents of the University of California
//
// NOTES
// ******************************
// *** DO NOT EDIT THIS FILE!//**
// ******************************
//
// It has been GENERATED by Gen_fmgrtab.sh
// from /Users/zlh/Downloads/postgresql-7.1.3/src/include/catalog/pg_proc.h
//
//===----------------------------------------------------------------------===//
#ifndef RDBMS_UTILS_FMGR_OIDS_H_
#define RDBMS_UTILS_FMGR_OIDS_H_

// Constant macros for the OIDs of entries in pg_proc.
//
// NOTE: macros are named after the prosrc value, ie the actual C name
// of the implementing function, not the proname which may be overloaded.
// For example, we want to be able to assign different macro names to both
// char_text() and int4_text() even though these both appear with proname
// 'text'.  If the same C function appears in more than one pg_proc entry,
// its equivalent macro will be defined with the lowest OID among those
// entries.
#define F_BYTEAOUT               31
#define F_CHAROUT                33
#define F_NAMEIN                 34
#define F_NAMEOUT                35
#define F_INT2IN                 38
#define F_INT2OUT                39
#define F_INT2VECTORIN           40
#define F_INT2VECTOROUT          41
#define F_INT4IN                 42
#define F_INT4OUT                43
#define F_REGPROCIN              44
#define F_REGPROCOUT             45
#define F_TEXTIN                 46
#define F_TEXTOUT                47
#define F_TIDIN                  48
#define F_TIDOUT                 49
#define F_XIDIN                  50
#define F_XIDOUT                 51
#define F_CIDIN                  52
#define F_CIDOUT                 53
#define F_OIDVECTORIN            54
#define F_OIDVECTOROUT           55
#define F_BOOLLT                 56
#define F_BOOLGT                 57
#define F_BOOLEQ                 60
#define F_CHAREQ                 61
#define F_NAMEEQ                 62
#define F_INT2EQ                 63
#define F_INT2LT                 64
#define F_INT4EQ                 65
#define F_INT4LT                 66
#define F_TEXTEQ                 67
#define F_XIDEQ                  68
#define F_CIDEQ                  69
#define F_CHARNE                 70
#define F_CHARLE                 72
#define F_CHARGT                 73
#define F_CHARGE                 74
#define F_CHARMUL                77
#define F_CHARDIV                78
#define F_NAMEREGEXEQ            79
#define F_BOOLNE                 84
#define F_PGSQL_VERSION          89
#define F_INT8FAC                100
#define F_EQSEL                  101
#define F_NEQSEL                 102
#define F_SCALARLTSEL            103
#define F_SCALARGTSEL            104
#define F_EQJOINSEL              105
#define F_NEQJOINSEL             106
#define F_SCALARLTJOINSEL        107
#define F_SCALARGTJOINSEL        108
#define F_INT4_TEXT              112
#define F_INT2_TEXT              113
#define F_OID_TEXT               114
#define F_BOX_ABOVE              115
#define F_BOX_BELOW              116
#define F_POINT_IN               117
#define F_POINT_OUT              118
#define F_LSEG_IN                119
#define F_LSEG_OUT               120
#define F_PATH_IN                121
#define F_PATH_OUT               122
#define F_BOX_IN                 123
#define F_BOX_OUT                124
#define F_BOX_OVERLAP            125
#define F_BOX_GE                 126
#define F_BOX_GT                 127
#define F_BOX_EQ                 128
#define F_BOX_LT                 129
#define F_BOX_LE                 130
#define F_POINT_ABOVE            131
#define F_POINT_LEFT             132
#define F_POINT_RIGHT            133
#define F_POINT_BELOW            134
#define F_POINT_EQ               135
#define F_ON_PB                  136
#define F_ON_PPATH               137
#define F_BOX_CENTER             138
#define F_AREASEL                139
#define F_AREAJOINSEL            140
#define F_INT4MUL                141
#define F_INT4FAC                142
#define F_INT4NE                 144
#define F_INT2NE                 145
#define F_INT2GT                 146
#define F_INT4GT                 147
#define F_INT2LE                 148
#define F_INT4LE                 149
#define F_INT4GE                 150
#define F_INT2GE                 151
#define F_INT2MUL                152
#define F_INT2DIV                153
#define F_INT4DIV                154
#define F_INT2MOD                155
#define F_INT4MOD                156
#define F_TEXTNE                 157
#define F_INT24EQ                158
#define F_INT42EQ                159
#define F_INT24LT                160
#define F_INT42LT                161
#define F_INT24GT                162
#define F_INT42GT                163
#define F_INT24NE                164
#define F_INT42NE                165
#define F_INT24LE                166
#define F_INT42LE                167
#define F_INT24GE                168
#define F_INT42GE                169
#define F_INT24MUL               170
#define F_INT42MUL               171
#define F_INT24DIV               172
#define F_INT42DIV               173
#define F_INT24MOD               174
#define F_INT42MOD               175
#define F_INT2PL                 176
#define F_INT4PL                 177
#define F_INT24PL                178
#define F_INT42PL                179
#define F_INT2MI                 180
#define F_INT4MI                 181
#define F_INT24MI                182
#define F_INT42MI                183
#define F_OIDEQ                  184
#define F_OIDNE                  185
#define F_BOX_SAME               186
#define F_BOX_CONTAIN            187
#define F_BOX_LEFT               188
#define F_BOX_OVERLEFT           189
#define F_BOX_OVERRIGHT          190
#define F_BOX_RIGHT              191
#define F_BOX_CONTAINED          192
#define F_RT_BOX_UNION           193
#define F_RT_BOX_INTER           194
#define F_RT_BOX_SIZE            195
#define F_RT_BIGBOX_SIZE         196
#define F_RT_POLY_UNION          197
#define F_RT_POLY_INTER          198
#define F_RT_POLY_SIZE           199
#define F_FLOAT4IN               200
#define F_FLOAT4OUT              201
#define F_FLOAT4MUL              202
#define F_FLOAT4DIV              203
#define F_FLOAT4PL               204
#define F_FLOAT4MI               205
#define F_FLOAT4UM               206
#define F_FLOAT4ABS              207
#define F_FLOAT4_ACCUM           208
#define F_FLOAT4LARGER           209
#define F_FLOAT4SMALLER          211
#define F_INT4UM                 212
#define F_INT2UM                 213
#define F_FLOAT8IN               214
#define F_FLOAT8OUT              215
#define F_FLOAT8MUL              216
#define F_FLOAT8DIV              217
#define F_FLOAT8PL               218
#define F_FLOAT8MI               219
#define F_FLOAT8UM               220
#define F_FLOAT8ABS              221
#define F_FLOAT8_ACCUM           222
#define F_FLOAT8LARGER           223
#define F_FLOAT8SMALLER          224
#define F_LSEG_CENTER            225
#define F_PATH_CENTER            226
#define F_POLY_CENTER            227
#define F_DROUND                 228
#define F_DTRUNC                 229
#define F_DSQRT                  230
#define F_DCBRT                  231
#define F_DPOW                   232
#define F_DEXP                   233
#define F_DLOG1                  234
#define F_I2TOD                  235
#define F_I2TOF                  236
#define F_DTOI2                  237
#define F_FTOI2                  238
#define F_LINE_DISTANCE          239
#define F_NABSTIMEIN             240
#define F_NABSTIMEOUT            241
#define F_RELTIMEIN              242
#define F_RELTIMEOUT             243
#define F_TIMEPL                 244
#define F_TIMEMI                 245
#define F_TINTERVALIN            246
#define F_TINTERVALOUT           247
#define F_INTINTERVAL            248
#define F_TINTERVALREL           249
#define F_TIMENOW                250
#define F_ABSTIMEEQ              251
#define F_ABSTIMENE              252
#define F_ABSTIMELT              253
#define F_ABSTIMEGT              254
#define F_ABSTIMELE              255
#define F_ABSTIMEGE              256
#define F_RELTIMEEQ              257
#define F_RELTIMENE              258
#define F_RELTIMELT              259
#define F_RELTIMEGT              260
#define F_RELTIMELE              261
#define F_RELTIMEGE              262
#define F_TINTERVALSAME          263
#define F_TINTERVALCT            264
#define F_TINTERVALOV            265
#define F_TINTERVALLENEQ         266
#define F_TINTERVALLENNE         267
#define F_TINTERVALLENLT         268
#define F_TINTERVALLENGT         269
#define F_TINTERVALLENLE         270
#define F_TINTERVALLENGE         271
#define F_TINTERVALSTART         272
#define F_TINTERVALEND           273
#define F_TIMEOFDAY              274
#define F_ABSTIME_FINITE         275
#define F_INT2FAC                276
#define F_INTER_SL               277
#define F_INTER_LB               278
#define F_FLOAT48MUL             279
#define F_FLOAT48DIV             280
#define F_FLOAT48PL              281
#define F_FLOAT48MI              282
#define F_FLOAT84MUL             283
#define F_FLOAT84DIV             284
#define F_FLOAT84PL              285
#define F_FLOAT84MI              286
#define F_FLOAT4EQ               287
#define F_FLOAT4NE               288
#define F_FLOAT4LT               289
#define F_FLOAT4LE               290
#define F_FLOAT4GT               291
#define F_FLOAT4GE               292
#define F_FLOAT8EQ               293
#define F_FLOAT8NE               294
#define F_FLOAT8LT               295
#define F_FLOAT8LE               296
#define F_FLOAT8GT               297
#define F_FLOAT8GE               298
#define F_FLOAT48EQ              299
#define F_FLOAT48NE              300
#define F_FLOAT48LT              301
#define F_FLOAT48LE              302
#define F_FLOAT48GT              303
#define F_FLOAT48GE              304
#define F_FLOAT84EQ              305
#define F_FLOAT84NE              306
#define F_FLOAT84LT              307
#define F_FLOAT84LE              308
#define F_FLOAT84GT              309
#define F_FLOAT84GE              310
#define F_FTOD                   311
#define F_DTOF                   312
#define F_I2TOI4                 313
#define F_I4TOI2                 314
#define F_INT2VECTOREQ           315
#define F_I4TOD                  316
#define F_DTOI4                  317
#define F_I4TOF                  318
#define F_FTOI4                  319
#define F_RTINSERT               320
#define F_RTDELETE               321
#define F_RTGETTUPLE             322
#define F_RTBUILD                323
#define F_RTBEGINSCAN            324
#define F_RTENDSCAN              325
#define F_RTMARKPOS              326
#define F_RTRESTRPOS             327
#define F_RTRESCAN               328
#define F_BTGETTUPLE             330
#define F_BTINSERT               331
#define F_BTDELETE               332
#define F_BTBEGINSCAN            333
#define F_BTRESCAN               334
#define F_BTENDSCAN              335
#define F_BTMARKPOS              336
#define F_BTRESTRPOS             337
#define F_BTBUILD                338
#define F_POLY_SAME              339
#define F_POLY_CONTAIN           340
#define F_POLY_LEFT              341
#define F_POLY_OVERLEFT          342
#define F_POLY_OVERRIGHT         343
#define F_POLY_RIGHT             344
#define F_POLY_CONTAINED         345
#define F_POLY_OVERLAP           346
#define F_POLY_IN                347
#define F_POLY_OUT               348
#define F_BTINT2CMP              350
#define F_BTINT4CMP              351
#define F_BTFLOAT4CMP            354
#define F_BTFLOAT8CMP            355
#define F_BTOIDCMP               356
#define F_BTABSTIMECMP           357
#define F_BTCHARCMP              358
#define F_BTNAMECMP              359
#define F_BTTEXTCMP              360
#define F_LSEG_DISTANCE          361
#define F_LSEG_INTERPT           362
#define F_DIST_PS                363
#define F_DIST_PB                364
#define F_DIST_SB                365
#define F_CLOSE_PS               366
#define F_CLOSE_PB               367
#define F_CLOSE_SB               368
#define F_ON_PS                  369
#define F_PATH_DISTANCE          370
#define F_DIST_PPATH             371
#define F_ON_SB                  372
#define F_INTER_SB               373
#define F_HASHMACADDR            399
#define F_BTOIDVECTORCMP         404
#define F_NAME_TEXT              406
#define F_TEXT_NAME              407
#define F_NAME_BPCHAR            408
#define F_BPCHAR_NAME            409
#define F_MACADDR_IN             436
#define F_MACADDR_OUT            437
#define F_HASHCOSTESTIMATE       438
#define F_HASHGETTUPLE           440
#define F_HASHINSERT             441
#define F_HASHDELETE             442
#define F_HASHBEGINSCAN          443
#define F_HASHRESCAN             444
#define F_HASHENDSCAN            445
#define F_HASHMARKPOS            446
#define F_HASHRESTRPOS           447
#define F_HASHBUILD              448
#define F_HASHINT2               449
#define F_HASHINT4               450
#define F_HASHFLOAT4             451
#define F_HASHFLOAT8             452
#define F_HASHOID                453
#define F_HASHCHAR               454
#define F_HASHNAME               455
#define F_HASHVARLENA            456
#define F_HASHOIDVECTOR          457
#define F_TEXT_LARGER            458
#define F_TEXT_SMALLER           459
#define F_INT8IN                 460
#define F_INT8OUT                461
#define F_INT8UM                 462
#define F_INT8PL                 463
#define F_INT8MI                 464
#define F_INT8MUL                465
#define F_INT8DIV                466
#define F_INT8EQ                 467
#define F_INT8NE                 468
#define F_INT8LT                 469
#define F_INT8GT                 470
#define F_INT8LE                 471
#define F_INT8GE                 472
#define F_INT84EQ                474
#define F_INT84NE                475
#define F_INT84LT                476
#define F_INT84GT                477
#define F_INT84LE                478
#define F_INT84GE                479
#define F_INT84                  480
#define F_INT48                  481
#define F_I8TOD                  482
#define F_DTOI8                  483
#define F_NETWORK_ABBREV         605
#define F_OIDVECTORNE            619
#define F_INT44OUT               653
#define F_NAMELT                 655
#define F_NAMELE                 656
#define F_NAMEGT                 657
#define F_NAMEGE                 658
#define F_NAMENE                 659
#define F_BPCHAR                 668
#define F_VARCHAR                669
#define F_MKTINTERVAL            676
#define F_OIDVECTORLT            677
#define F_OIDVECTORLE            678
#define F_OIDVECTOREQ            679
#define F_OIDVECTORGE            680
#define F_OIDVECTORGT            681
#define F_NETWORK_NETWORK        683
#define F_NETWORK_NETMASK        696
#define F_NETWORK_MASKLEN        697
#define F_NETWORK_BROADCAST      698
#define F_NETWORK_HOST           699
#define F_CURRENT_USER           710
#define F_USERFNTEST             711
#define F_OIDRAND                713
#define F_OIDSRAND               715
#define F_OIDLT                  716
#define F_OIDLE                  717
#define F_BYTEAOCTETLEN          720
#define F_BYTEAGETBYTE           721
#define F_BYTEASETBYTE           722
#define F_BYTEAGETBIT            723
#define F_BYTEASETBIT            724
#define F_DIST_PL                725
#define F_DIST_LB                726
#define F_DIST_SL                727
#define F_DIST_CPOLY             728
#define F_POLY_DISTANCE          729
#define F_NETWORK_SHOW           730
#define F_TEXT_LT                740
#define F_TEXT_LE                741
#define F_TEXT_GT                742
#define F_TEXT_GE                743
#define F_ARRAY_EQ               744
#define F_SESSION_USER           746
#define F_ARRAY_DIMS             747
#define F_TEXT_DATE              748
#define F_DATE_TEXT              749
#define F_ARRAY_IN               750
#define F_ARRAY_OUT              751
#define F_MACADDR_TEXT           752
#define F_MACADDR_TRUNC          753
#define F_SMGRIN                 760
#define F_SMGROUT                761
#define F_SMGREQ                 762
#define F_SMGRNE                 763
#define F_LO_IMPORT              764
#define F_LO_EXPORT              765
#define F_INT4INC                766
#define F_TEXT_MACADDR           767
#define F_INT4LARGER             768
#define F_INT4SMALLER            769
#define F_INT2LARGER             770
#define F_INT2SMALLER            771
#define F_GISTCOSTESTIMATE       772
#define F_GISTGETTUPLE           774
#define F_GISTINSERT             775
#define F_GISTDELETE             776
#define F_GISTBEGINSCAN          777
#define F_GISTRESCAN             778
#define F_GISTENDSCAN            779
#define F_GISTMARKPOS            780
#define F_GISTRESTRPOS           781
#define F_GISTBUILD              782
#define F_TINTERVALEQ            784
#define F_TINTERVALNE            785
#define F_TINTERVALLT            786
#define F_TINTERVALGT            787
#define F_TINTERVALLE            788
#define F_TINTERVALGE            789
#define F_TEXT_OID               817
#define F_TEXT_INT2              818
#define F_TEXT_INT4              819
#define F_MACADDR_EQ             830
#define F_MACADDR_LT             831
#define F_MACADDR_LE             832
#define F_MACADDR_GT             833
#define F_MACADDR_GE             834
#define F_MACADDR_NE             835
#define F_MACADDR_CMP            836
#define F_TEXT_TIME              837
#define F_TEXT_FLOAT8            838
#define F_TEXT_FLOAT4            839
#define F_FLOAT8_TEXT            840
#define F_FLOAT4_TEXT            841
#define F_BTINT8CMP              842
#define F_CASH_MUL_FLT4          846
#define F_CASH_DIV_FLT4          847
#define F_FLT4_MUL_CASH          848
#define F_TEXTPOS                849
#define F_TEXTLIKE               850
#define F_TEXTNLIKE              851
#define F_INT48EQ                852
#define F_INT48NE                853
#define F_INT48LT                854
#define F_INT48GT                855
#define F_INT48LE                856
#define F_INT48GE                857
#define F_NAMELIKE               858
#define F_NAMENLIKE              859
#define F_CHAR_BPCHAR            860
#define F_BPCHAR_CHAR            861
#define F_INT4_MUL_CASH          862
#define F_INT2_MUL_CASH          863
#define F_CASH_MUL_INT4          864
#define F_CASH_DIV_INT4          865
#define F_CASH_MUL_INT2          866
#define F_CASH_DIV_INT2          867
#define F_LOWER                  870
#define F_UPPER                  871
#define F_INITCAP                872
#define F_LPAD                   873
#define F_RPAD                   874
#define F_LTRIM                  875
#define F_RTRIM                  876
#define F_TEXT_SUBSTR            877
#define F_TRANSLATE              878
#define F_BTRIM                  884
#define F_CASH_IN                886
#define F_CASH_OUT               887
#define F_CASH_EQ                888
#define F_CASH_NE                889
#define F_CASH_LT                890
#define F_CASH_LE                891
#define F_CASH_GT                892
#define F_CASH_GE                893
#define F_CASH_PL                894
#define F_CASH_MI                895
#define F_CASH_MUL_FLT8          896
#define F_CASH_DIV_FLT8          897
#define F_CASHLARGER             898
#define F_CASHSMALLER            899
#define F_INET_IN                910
#define F_INET_OUT               911
#define F_FLT8_MUL_CASH          919
#define F_NETWORK_EQ             920
#define F_NETWORK_LT             921
#define F_NETWORK_LE             922
#define F_NETWORK_GT             923
#define F_NETWORK_GE             924
#define F_NETWORK_NE             925
#define F_NETWORK_CMP            926
#define F_NETWORK_SUB            927
#define F_NETWORK_SUBEQ          928
#define F_NETWORK_SUP            929
#define F_NETWORK_SUPEQ          930
#define F_TEXT_TIMETZ            938
#define F_TIMETZ_TEXT            939
#define F_TEXT_CHAR              944
#define F_INT8MOD                945
#define F_CHAR_TEXT              946
#define F_TIME_TEXT              948
#define F_HASHINT8               949
#define F_ISTRUE                 950
#define F_ISFALSE                951
#define F_LO_OPEN                952
#define F_LO_CLOSE               953
#define F_LOREAD                 954
#define F_LOWRITE                955
#define F_LO_LSEEK               956
#define F_LO_CREAT               957
#define F_LO_TELL                958
#define F_ON_PL                  959
#define F_ON_SL                  960
#define F_CLOSE_PL               961
#define F_CLOSE_SL               962
#define F_CLOSE_LB               963
#define F_LO_UNLINK              964
#define F_REGPROCTOOID           972
#define F_PATH_INTER             973
#define F_BOX_AREA               975
#define F_BOX_WIDTH              976
#define F_BOX_HEIGHT             977
#define F_BOX_DISTANCE           978
#define F_BOX_INTERSECT          980
#define F_BOX_DIAGONAL           981
#define F_PATH_N_LT              982
#define F_PATH_N_GT              983
#define F_PATH_N_EQ              984
#define F_PATH_N_LE              985
#define F_PATH_N_GE              986
#define F_PATH_LENGTH            987
#define F_POINT_NE               988
#define F_POINT_VERT             989
#define F_POINT_HORIZ            990
#define F_POINT_DISTANCE         991
#define F_POINT_SLOPE            992
#define F_LSEG_CONSTRUCT         993
#define F_LSEG_INTERSECT         994
#define F_LSEG_PARALLEL          995
#define F_LSEG_PERP              996
#define F_LSEG_VERTICAL          997
#define F_LSEG_HORIZONTAL        998
#define F_LSEG_EQ                999
#define F_TIMESTAMP_IZONE        1026
#define F_NULLVALUE              1029
#define F_NONNULLVALUE           1030
#define F_ACLITEMIN              1031
#define F_ACLITEMOUT             1032
#define F_ACLINSERT              1035
#define F_ACLREMOVE              1036
#define F_ACLCONTAINS            1037
#define F_SETEVAL                1038
#define F_GETDATABASEENCODING    1039
#define F_BPCHARIN               1044
#define F_BPCHAROUT              1045
#define F_VARCHARIN              1046
#define F_VARCHAROUT             1047
#define F_BPCHAREQ               1048
#define F_BPCHARLT               1049
#define F_BPCHARLE               1050
#define F_BPCHARGT               1051
#define F_BPCHARGE               1052
#define F_BPCHARNE               1053
#define F_VARCHAREQ              1070
#define F_VARCHARLT              1071
#define F_VARCHARLE              1072
#define F_VARCHARGT              1073
#define F_VARCHARGE              1074
#define F_VARCHARNE              1075
#define F_BPCHARCMP              1078
#define F_VARCHARCMP             1079
#define F_HASHBPCHAR             1080
#define F_FORMAT_TYPE            1081
#define F_DATE_IN                1084
#define F_DATE_OUT               1085
#define F_DATE_EQ                1086
#define F_DATE_LT                1087
#define F_DATE_LE                1088
#define F_DATE_GT                1089
#define F_DATE_GE                1090
#define F_DATE_NE                1091
#define F_DATE_CMP               1092
#define F_TIME_LT                1102
#define F_TIME_LE                1103
#define F_TIME_GT                1104
#define F_TIME_GE                1105
#define F_TIME_NE                1106
#define F_TIME_CMP               1107
#define F_DATE_LARGER            1138
#define F_DATE_SMALLER           1139
#define F_DATE_MI                1140
#define F_DATE_PLI               1141
#define F_DATE_MII               1142
#define F_TIME_IN                1143
#define F_TIME_OUT               1144
#define F_TIME_EQ                1145
#define F_CIRCLE_ADD_PT          1146
#define F_CIRCLE_SUB_PT          1147
#define F_CIRCLE_MUL_PT          1148
#define F_CIRCLE_DIV_PT          1149
#define F_TIMESTAMP_IN           1150
#define F_TIMESTAMP_OUT          1151
#define F_TIMESTAMP_EQ           1152
#define F_TIMESTAMP_NE           1153
#define F_TIMESTAMP_LT           1154
#define F_TIMESTAMP_LE           1155
#define F_TIMESTAMP_GE           1156
#define F_TIMESTAMP_GT           1157
#define F_TIMESTAMP_ZONE         1159
#define F_INTERVAL_IN            1160
#define F_INTERVAL_OUT           1161
#define F_INTERVAL_EQ            1162
#define F_INTERVAL_NE            1163
#define F_INTERVAL_LT            1164
#define F_INTERVAL_LE            1165
#define F_INTERVAL_GE            1166
#define F_INTERVAL_GT            1167
#define F_INTERVAL_UM            1168
#define F_INTERVAL_PL            1169
#define F_INTERVAL_MI            1170
#define F_TIMESTAMP_PART         1171
#define F_INTERVAL_PART          1172
#define F_ABSTIME_TIMESTAMP      1173
#define F_DATE_TIMESTAMP         1174
#define F_DATETIME_TIMESTAMP     1176
#define F_RELTIME_INTERVAL       1177
#define F_TIMESTAMP_DATE         1178
#define F_ABSTIME_DATE           1179
#define F_TIMESTAMP_ABSTIME      1180
#define F_TIMESTAMP_MI           1188
#define F_TIMESTAMP_PL_SPAN      1189
#define F_TIMESTAMP_MI_SPAN      1190
#define F_TEXT_TIMESTAMP         1191
#define F_TIMESTAMP_TEXT         1192
#define F_INTERVAL_TEXT          1193
#define F_INTERVAL_RELTIME       1194
#define F_TIMESTAMP_SMALLER      1195
#define F_TIMESTAMP_LARGER       1196
#define F_INTERVAL_SMALLER       1197
#define F_INTERVAL_LARGER        1198
#define F_TIMESTAMP_AGE          1199
#define F_INT4RELTIME            1200
#define F_TIMESTAMP_TRUNC        1217
#define F_INTERVAL_TRUNC         1218
#define F_INT8ABS                1230
#define F_INT8LARGER             1236
#define F_INT8SMALLER            1237
#define F_TEXTICREGEXEQ          1238
#define F_TEXTICREGEXNE          1239
#define F_NAMEICREGEXEQ          1240
#define F_NAMEICREGEXNE          1241
#define F_BOOLIN                 1242
#define F_BOOLOUT                1243
#define F_BYTEAIN                1244
#define F_CHARIN                 1245
#define F_CHARLT                 1246
#define F_CHARPL                 1248
#define F_CHARMI                 1250
#define F_INT4ABS                1251
#define F_NAMEREGEXNE            1252
#define F_INT2ABS                1253
#define F_TEXTREGEXEQ            1254
#define F_TEXTREGEXNE            1256
#define F_TEXTLEN                1257
#define F_TEXTCAT                1258
#define F_TEXT_INTERVAL          1263
#define F_RTCOSTESTIMATE         1265
#define F_CIDR_IN                1267
#define F_BTCOSTESTIMATE         1268
#define F_OVERLAPS_TIMETZ        1271
#define F_CASH_WORDS             1273
#define F_INT84PL                1274
#define F_INT84MI                1275
#define F_INT84MUL               1276
#define F_INT84DIV               1277
#define F_INT48PL                1278
#define F_INT48MI                1279
#define F_INT48MUL               1280
#define F_INT48DIV               1281
#define F_QUOTE_IDENT            1282
#define F_QUOTE_LITERAL          1283
#define F_INT4NOTIN              1285
#define F_OIDNOTIN               1286
#define F_INT44IN                1287
#define F_INT8_TEXT              1288
#define F_TEXT_INT8              1289
#define F__BPCHAR                1290
#define F__VARCHAR               1291
#define F_TIDEQ                  1292
#define F_CURRTID_BYRELOID       1293
#define F_CURRTID_BYRELNAME      1294
#define F_PG_CHAR_TO_ENCODING    1295
#define F_DATETIMETZ_TIMESTAMP   1297
#define F_NOW                    1299
#define F_POSITIONSEL            1300
#define F_POSITIONJOINSEL        1301
#define F_CONTSEL                1302
#define F_CONTJOINSEL            1303
#define F_OVERLAPS_TIMESTAMP     1304
#define F_OVERLAPS_TIME          1308
#define F_TIMESTAMP_CMP          1314
#define F_INTERVAL_CMP           1315
#define F_TIMESTAMP_TIME         1316
#define F_BPCHARLEN              1318
#define F_VARCHARLEN             1319
#define F_INTERVAL_DIV           1326
#define F_DLOG10                 1339
#define F_OIDVECTORTYPES         1349
#define F_TIMETZ_IN              1350
#define F_TIMETZ_OUT             1351
#define F_TIMETZ_EQ              1352
#define F_TIMETZ_NE              1353
#define F_TIMETZ_LT              1354
#define F_TIMETZ_LE              1355
#define F_TIMETZ_GE              1356
#define F_TIMETZ_GT              1357
#define F_TIMETZ_CMP             1358
#define F_TIME_INTERVAL          1370
#define F_TEXTOCTETLEN           1374
#define F_BPCHAROCTETLEN         1375
#define F_VARCHAROCTETLEN        1376
#define F_TIME_LARGER            1377
#define F_TIME_SMALLER           1378
#define F_TIMETZ_LARGER          1379
#define F_TIMETZ_SMALLER         1380
#define F_TIMESTAMP_TIMETZ       1388
#define F_TIMESTAMP_FINITE       1389
#define F_INTERVAL_FINITE        1390
#define F_LINE_PARALLEL          1412
#define F_LINE_PERP              1413
#define F_LINE_VERTICAL          1414
#define F_LINE_HORIZONTAL        1415
#define F_CIRCLE_CENTER          1416
#define F_ISNOTTRUE              1417
#define F_ISNOTFALSE             1418
#define F_INTERVAL_TIME          1419
#define F_POINTS_BOX             1421
#define F_BOX_ADD                1422
#define F_BOX_SUB                1423
#define F_BOX_MUL                1424
#define F_BOX_DIV                1425
#define F_CIDR_OUT               1427
#define F_POLY_CONTAIN_PT        1428
#define F_PT_CONTAINED_POLY      1429
#define F_PATH_ISCLOSED          1430
#define F_PATH_ISOPEN            1431
#define F_PATH_NPOINTS           1432
#define F_PATH_CLOSE             1433
#define F_PATH_OPEN              1434
#define F_PATH_ADD               1435
#define F_PATH_ADD_PT            1436
#define F_PATH_SUB_PT            1437
#define F_PATH_MUL_PT            1438
#define F_PATH_DIV_PT            1439
#define F_CONSTRUCT_POINT        1440
#define F_POINT_ADD              1441
#define F_POINT_SUB              1442
#define F_POINT_MUL              1443
#define F_POINT_DIV              1444
#define F_POLY_NPOINTS           1445
#define F_POLY_BOX               1446
#define F_POLY_PATH              1447
#define F_BOX_POLY               1448
#define F_PATH_POLY              1449
#define F_CIRCLE_IN              1450
#define F_CIRCLE_OUT             1451
#define F_CIRCLE_SAME            1452
#define F_CIRCLE_CONTAIN         1453
#define F_CIRCLE_LEFT            1454
#define F_CIRCLE_OVERLEFT        1455
#define F_CIRCLE_OVERRIGHT       1456
#define F_CIRCLE_RIGHT           1457
#define F_CIRCLE_CONTAINED       1458
#define F_CIRCLE_OVERLAP         1459
#define F_CIRCLE_BELOW           1460
#define F_CIRCLE_ABOVE           1461
#define F_CIRCLE_EQ              1462
#define F_CIRCLE_NE              1463
#define F_CIRCLE_LT              1464
#define F_CIRCLE_GT              1465
#define F_CIRCLE_LE              1466
#define F_CIRCLE_GE              1467
#define F_CIRCLE_AREA            1468
#define F_CIRCLE_DIAMETER        1469
#define F_CIRCLE_RADIUS          1470
#define F_CIRCLE_DISTANCE        1471
#define F_CR_CIRCLE              1473
#define F_POLY_CIRCLE            1474
#define F_CIRCLE_POLY            1475
#define F_DIST_PC                1476
#define F_CIRCLE_CONTAIN_PT      1477
#define F_PT_CONTAINED_CIRCLE    1478
#define F_BOX_CIRCLE             1479
#define F_CIRCLE_BOX             1480
#define F_LSEG_NE                1482
#define F_LSEG_LT                1483
#define F_LSEG_LE                1484
#define F_LSEG_GT                1485
#define F_LSEG_GE                1486
#define F_LSEG_LENGTH            1487
#define F_CLOSE_LS               1488
#define F_CLOSE_LSEG             1489
#define F_LINE_IN                1490
#define F_LINE_OUT               1491
#define F_LINE_EQ                1492
#define F_LINE_CONSTRUCT_PP      1493
#define F_LINE_INTERPT           1494
#define F_LINE_INTERSECT         1495
#define F_ZPBIT_IN               1564
#define F_ZPBIT_OUT              1565
#define F_NEXTVAL                1574
#define F_CURRVAL                1575
#define F_SETVAL                 1576
#define F_VARBIT_IN              1579
#define F_VARBIT_OUT             1580
#define F_BITEQ                  1581
#define F_BITNE                  1582
#define F_BITGE                  1592
#define F_BITGT                  1593
#define F_BITLE                  1594
#define F_BITLT                  1595
#define F_BITCMP                 1596
#define F_PG_ENCODING_TO_CHAR    1597
#define F_DRANDOM                1598
#define F_SETSEED                1599
#define F_DASIN                  1600
#define F_DACOS                  1601
#define F_DATAN                  1602
#define F_DATAN2                 1603
#define F_DSIN                   1604
#define F_DCOS                   1605
#define F_DTAN                   1606
#define F_DCOT                   1607
#define F_DEGREES                1608
#define F_RADIANS                1609
#define F_DPI                    1610
#define F_INTERVAL_MUL           1618
#define F_ASCII                  1620
#define F_CHR                    1621
#define F_REPEAT                 1622
#define F_MUL_D_INTERVAL         1624
#define F_TEXTICLIKE             1633
#define F_TEXTICNLIKE            1634
#define F_NAMEICLIKE             1635
#define F_NAMEICNLIKE            1636
#define F_LIKE_ESCAPE            1637
#define F_OIDGT                  1638
#define F_OIDGE                  1639
#define F_PG_GET_RULEDEF         1640
#define F_PG_GET_VIEWDEF         1641
#define F_PG_GET_USERBYID        1642
#define F_PG_GET_INDEXDEF        1643
#define F_RI_FKEY_CHECK_INS      1644
#define F_RI_FKEY_CHECK_UPD      1645
#define F_RI_FKEY_CASCADE_DEL    1646
#define F_RI_FKEY_CASCADE_UPD    1647
#define F_RI_FKEY_RESTRICT_DEL   1648
#define F_RI_FKEY_RESTRICT_UPD   1649
#define F_RI_FKEY_SETNULL_DEL    1650
#define F_RI_FKEY_SETNULL_UPD    1651
#define F_RI_FKEY_SETDEFAULT_DEL 1652
#define F_RI_FKEY_SETDEFAULT_UPD 1653
#define F_RI_FKEY_NOACTION_DEL   1654
#define F_RI_FKEY_NOACTION_UPD   1655
#define F_BITAND                 1673
#define F_BITOR                  1674
#define F_BITXOR                 1675
#define F_BITNOT                 1676
#define F_BITSHIFTLEFT           1677
#define F_BITSHIFTRIGHT          1678
#define F_BITCAT                 1679
#define F_BITSUBSTR              1680
#define F_BITLENGTH              1681
#define F_BITOCTETLENGTH         1682
#define F_BITFROMINT4            1683
#define F_BITTOINT4              1684
#define F_ZPBIT                  1685
#define F__ZPBIT                 1686
#define F_VARBIT                 1687
#define F__VARBIT                1688
#define F_UPDATE_PG_PWD          1689
#define F_BOOLLE                 1691
#define F_BOOLGE                 1692
#define F_BTBOOLCMP              1693
#define F_TIMETZ_HASH            1696
#define F_INTERVAL_HASH          1697
#define F_BITPOSITION            1698
#define F_NUMERIC_IN             1701
#define F_NUMERIC_OUT            1702
#define F_NUMERIC                1703
#define F_NUMERIC_ABS            1704
#define F_NUMERIC_SIGN           1706
#define F_NUMERIC_ROUND          1707
#define F_NUMERIC_TRUNC          1709
#define F_NUMERIC_CEIL           1711
#define F_NUMERIC_FLOOR          1712
#define F_NUMERIC_EQ             1718
#define F_NUMERIC_NE             1719
#define F_NUMERIC_GT             1720
#define F_NUMERIC_GE             1721
#define F_NUMERIC_LT             1722
#define F_NUMERIC_LE             1723
#define F_NUMERIC_ADD            1724
#define F_NUMERIC_SUB            1725
#define F_NUMERIC_MUL            1726
#define F_NUMERIC_DIV            1727
#define F_NUMERIC_MOD            1728
#define F_NUMERIC_SQRT           1730
#define F_NUMERIC_EXP            1732
#define F_NUMERIC_LN             1734
#define F_NUMERIC_LOG            1736
#define F_NUMERIC_POWER          1738
#define F_INT4_NUMERIC           1740
#define F_FLOAT4_NUMERIC         1742
#define F_FLOAT8_NUMERIC         1743
#define F_NUMERIC_INT4           1744
#define F_NUMERIC_FLOAT4         1745
#define F_NUMERIC_FLOAT8         1746
#define F_TIME_PL_INTERVAL       1747
#define F_TIME_MI_INTERVAL       1748
#define F_TIMETZ_PL_INTERVAL     1749
#define F_TIMETZ_MI_INTERVAL     1750
#define F_NUMERIC_INC            1764
#define F_SETVAL_AND_ISCALLED    1765
#define F_NUMERIC_SMALLER        1766
#define F_NUMERIC_LARGER         1767
#define F_NUMERIC_CMP            1769
#define F_TIMESTAMP_TO_CHAR      1770
#define F_NUMERIC_UMINUS         1771
#define F_NUMERIC_TO_CHAR        1772
#define F_INT4_TO_CHAR           1773
#define F_INT8_TO_CHAR           1774
#define F_FLOAT4_TO_CHAR         1775
#define F_FLOAT8_TO_CHAR         1776
#define F_NUMERIC_TO_NUMBER      1777
#define F_TO_TIMESTAMP           1778
#define F_NUMERIC_INT8           1779
#define F_TO_DATE                1780
#define F_INT8_NUMERIC           1781
#define F_INT2_NUMERIC           1782
#define F_NUMERIC_INT2           1783
#define F_OIDIN                  1798
#define F_OIDOUT                 1799
#define F_ICLIKESEL              1814
#define F_ICNLIKESEL             1815
#define F_ICLIKEJOINSEL          1816
#define F_ICNLIKEJOINSEL         1817
#define F_REGEXEQSEL             1818
#define F_LIKESEL                1819
#define F_ICREGEXEQSEL           1820
#define F_REGEXNESEL             1821
#define F_NLIKESEL               1822
#define F_ICREGEXNESEL           1823
#define F_REGEXEQJOINSEL         1824
#define F_LIKEJOINSEL            1825
#define F_ICREGEXEQJOINSEL       1826
#define F_REGEXNEJOINSEL         1827
#define F_NLIKEJOINSEL           1828
#define F_ICREGEXNEJOINSEL       1829
#define F_FLOAT8_AVG             1830
#define F_FLOAT8_VARIANCE        1831
#define F_FLOAT8_STDDEV          1832
#define F_NUMERIC_ACCUM          1833
#define F_INT2_ACCUM             1834
#define F_INT4_ACCUM             1835
#define F_INT8_ACCUM             1836
#define F_NUMERIC_AVG            1837
#define F_NUMERIC_VARIANCE       1838
#define F_NUMERIC_STDDEV         1839
#define F_INT2_SUM               1840
#define F_INT4_SUM               1841
#define F_INT8_SUM               1842
#define F_INTERVAL_ACCUM         1843
#define F_INTERVAL_AVG           1844
#define F_TO_ASCII_DEFAULT       1845
#define F_TO_ASCII_ENC           1846
#define F_TO_ASCII_ENCNAME       1847
#define F_INTERVAL_PL_TIME       1848
#define F_INT28EQ                1850
#define F_INT28NE                1851
#define F_INT28LT                1852
#define F_INT28GT                1853
#define F_INT28LE                1854
#define F_INT28GE                1855
#define F_INT82EQ                1856
#define F_INT82NE                1857
#define F_INT82LT                1858
#define F_INT82GT                1859
#define F_INT82LE                1860
#define F_INT82GE                1861
#define F_INT2AND                1892
#define F_INT2OR                 1893
#define F_INT2XOR                1894
#define F_INT2NOT                1895
#define F_INT2SHL                1896
#define F_INT2SHR                1897
#define F_INT4AND                1898
#define F_INT4OR                 1899
#define F_INT4XOR                1900
#define F_INT4NOT                1901
#define F_INT4SHL                1902
#define F_INT4SHR                1903
#define F_INT8AND                1904
#define F_INT8OR                 1905
#define F_INT8XOR                1906
#define F_INT8NOT                1907
#define F_INT8SHL                1908
#define F_INT8SHR                1909

#endif  // RDBMS_UTILS_FMGR_OIDS_H_
