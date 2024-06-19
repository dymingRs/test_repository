#ifndef _MAC_PORT_H_
#define _MAC_PORT_H_

void module_I_nss_ctr( mac_port_pin_state_t state );
void module_I_rst_ctr( mac_port_pin_state_t state );
bool module_I_busy_check( void );
void module_I_irq_polling( void );

void module_II_nss_ctr( mac_port_pin_state_t state );
void module_II_rst_ctr( mac_port_pin_state_t state );
bool module_II_busy_check( void );
void module_II_irq_polling( void );


#endif
