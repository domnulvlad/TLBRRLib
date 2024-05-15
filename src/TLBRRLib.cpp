//todo: update function to manage request pulse (3ms HIGH on ENA)

#include "TLBRRLib.h"

TLBRRLib::TLBRRLib(uint8_t ENA_pin, uint8_t CLK_pin, uint8_t DAT_pin,
                   attachInterruptENA_type attachInterruptENA,
                   detachInterruptENA_type detachInterruptENA,
                   attachInterruptCLK_type attachInterruptCLK,
                   detachInterruptCLK_type detachInterruptCLK) :
  _ENA_pin (ENA_pin), _CLK_pin (CLK_pin), _DAT_pin (DAT_pin), //ENA, CLK, DAT pins
  _attachInterruptENA (attachInterruptENA),                   //attachInterrupt() for ENA
  _detachInterruptENA (detachInterruptENA),                   //detachInterrupt() for ENA
  _attachInterruptCLK (attachInterruptCLK),                   //attachInterrupt() for CLK
  _detachInterruptCLK (detachInterruptCLK)                    //detachInterrupt() for CLK
{}

void TLBRRLib::begin()
{
  //Configure the lines.
  pinMode(_ENA_pin, INPUT);
  pinMode(_CLK_pin, INPUT_PULLUP);
  pinMode(_DAT_pin, INPUT_PULLUP);
  
  //Wait for ENA to activate.
  attach_ENA_interrupt(RISING); //capture ENA going HIGH
}

void TLBRRLib::end()
{
  //Reset the lines to their default states.
  pinMode(_CLK_pin, INPUT);
  pinMode(_DAT_pin, INPUT);
  
  //Detach all interrupts.
  detach_ENA_interrupt(); //stop capturing ENA
  detach_CLK_interrupt(); //stop capturing CLK
}

void TLBRRLib::executeInterruptENA()
{
  //Determine which state ENA is in by the type of interrupt that was set.
  switch (_current_ENA_interrupt_type)
  {
    case RISING: //ENA is currently HIGH
      {
        //Start receiving.
        _current_bit = 0;
        _is_receiving = true;
        _new_data_in_internal_buffer = false;
        
        //Wait for data to be received or for ENA to deactivate.
        attach_CLK_interrupt(RISING); //capture CLK going HIGH
        attach_ENA_interrupt(FALLING); //capture ENA going LOW
      }
      break;
    
    case FALLING: //ENA is currently LOW
      {
        //Detach all interrupts.
        detach_ENA_interrupt(); //stop capturing ENA
        detach_CLK_interrupt(); //stop capturing CLK
        
        //If any data was received, verify it.
        if (_current_bit)
        {
          //Stop receiving.
          _is_receiving = false;
          
          //The data should be 18 bytes long and the checksum must match.
          if (((_current_bit / 8) == 18) && verify_radio_data())
          {
            //If the user buffer is not being used, copy the received data to it.
            if (!_using_user_buffer)
            {
              memcpy(_user_buffer, _internal_buffer, 18);
              _new_data_in_user_buffer = true;
              _new_data_in_internal_buffer = false;
            }
            //If it is being used, save the data for copying later.
            else
            {
              _new_data_in_internal_buffer = true;
            }
          }          
        }
        
        //Wait for ENA to activate.
        attach_ENA_interrupt(RISING); //capture ENA going HIGH
      }
      break;
    
    default:
      break;
  }
}

void TLBRRLib::executeInterruptCLK()
{
  //Determine which state CLK is in by the type of interrupt that was set.
  switch (_current_CLK_interrupt_type)
  {
    case RISING: //CLK is currently HIGH
      {
        //Read the current bit and save it in the buffer if there is enough space.
        uint8_t current_byte = _current_bit++ / 8;
        if (current_byte < sizeof(_internal_buffer))
        {
          _internal_buffer[current_byte] = (_internal_buffer[current_byte] << 1) | !digitalRead(_DAT_pin);
        }
      }
      break;
    
    case FALLING: //CLK is currently LOW
      break;
    
    default:
      break;
  }
}

bool TLBRRLib::hasData()
{
  //Clear the flag before returning.
  if (_new_data_in_user_buffer)
  {
    _new_data_in_user_buffer = false;
    return true;
  }
  return false;
}

void TLBRRLib::requestData()
{
  if (!_is_receiving)
  {
    //Detach all interrupts.
    detach_ENA_interrupt(); //stop capturing ENA
    
    request_data(); //pull ENA HIGH
    delay(3);
    release_ENA(); //stop controlling ENA
    
    //Wait for ENA to activate.
    attach_ENA_interrupt(RISING); //capture ENA going HIGH
  }
}

uint8_t* TLBRRLib::getBuffer()
{
  //Mark the user buffer as being used.
  _using_user_buffer = true;
  return _user_buffer;
}

void TLBRRLib::continueReceiving()
{
  //The user buffer is not being used anymore.
  _using_user_buffer = false;
  
  //If new data was received while the user buffer was being used, copy the data now.
  cli();
  if (_new_data_in_internal_buffer)
  {
    memcpy(_user_buffer, _internal_buffer, 18);
    _new_data_in_user_buffer = true;
    _new_data_in_internal_buffer = false;
  }
  sei();
}

bool TLBRRLib::verify_radio_data()
{
  //Add all bytes without the last one.
  uint8_t sum = 0;
  for (uint8_t i = 0; i < 17; i++)
  {
    sum += _internal_buffer[i];
  }

  //If the last byte is the same as the inverted sum, the data is valid.
  return ((sum ^ 0xFF) == _internal_buffer[17]);
}

void TLBRRLib::request_data()
{
  pinMode(_ENA_pin, OUTPUT);
  digitalWrite(_ENA_pin, HIGH);
}

void TLBRRLib::release_ENA()
{
  pinMode(_ENA_pin, INPUT);
}

void TLBRRLib::attach_ENA_interrupt(uint8_t mode)
{
  _current_ENA_interrupt_type = mode;
  _attachInterruptENA(mode);
}

void TLBRRLib::detach_ENA_interrupt()
{
  _current_ENA_interrupt_type = 0;
  _detachInterruptENA();
}

void TLBRRLib::attach_CLK_interrupt(uint8_t mode)
{
  _current_CLK_interrupt_type = mode;
  _attachInterruptCLK(mode);
}

void TLBRRLib::detach_CLK_interrupt()
{
  _current_CLK_interrupt_type = 0;
  _detachInterruptCLK();
}
