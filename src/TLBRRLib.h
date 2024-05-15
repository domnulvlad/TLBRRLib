#ifndef TLBRRLib_H
#define TLBRRLib_H

#include <Arduino.h>

class TLBRRLib
{
  public:
    //Function pointer types for callbacks
    using attachInterruptENA_type = void (*)(uint8_t mode);
    using detachInterruptENA_type = void (*)();
    using attachInterruptCLK_type = void (*)(uint8_t mode);
    using detachInterruptCLK_type = void (*)();
    
    //Constructor
    TLBRRLib(uint8_t ENA_pin, uint8_t CLK_pin, uint8_t DAT_pin,
             attachInterruptENA_type attachInterruptENA,
             detachInterruptENA_type detachInterruptENA,
             attachInterruptCLK_type attachInterruptCLK,
             detachInterruptCLK_type detachInterruptCLK);
    
    //Initialize the bus
    void begin();
    //Deinitialize the bus
    void end();
    
    //Call when interrupt fires
    void executeInterruptENA();
    void executeInterruptCLK();
    
    //Check if new data is available
    bool hasData();
    
    //Request data to be sent
    void requestData();
    
    //Get the buffer containing the received data and stop updating it
    uint8_t* getBuffer();
    
    //Resume updating the buffer
    void continueReceiving();
  
  private:
    //Communication lines
    uint8_t _ENA_pin;
    uint8_t _CLK_pin;
    uint8_t _DAT_pin;
    
    //Callbacks
    attachInterruptENA_type _attachInterruptENA;
    detachInterruptENA_type _detachInterruptENA;
    attachInterruptCLK_type _attachInterruptCLK;
    detachInterruptCLK_type _detachInterruptCLK;
    
    //Currently set interrupts
    uint8_t _current_ENA_interrupt_type, _current_CLK_interrupt_type;
    
    //Internal receive buffer
    uint8_t _internal_buffer[18], _current_bit;
    bool _new_data_in_internal_buffer;
    
    //User receive buffer
    uint8_t _user_buffer[18];
    bool _is_receiving, _new_data_in_user_buffer, _using_user_buffer;
    
    //Checksum verification
    bool verify_radio_data();
    
    //Helper functions
    void request_data();
    void release_ENA();
    void attach_ENA_interrupt(uint8_t mode);
    void detach_ENA_interrupt();
    void attach_CLK_interrupt(uint8_t mode);
    void detach_CLK_interrupt();
};

#endif
