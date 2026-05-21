#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

void unity_output_start();
void unity_output_char(char);

#define UNITY_OUTPUT_CHAR(a) unity_output_char(a)
#define UNITY_OUTPUT_START() unity_output_start()

#ifdef __cplusplus
}
#endif
#endif
