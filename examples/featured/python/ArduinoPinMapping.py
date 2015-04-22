####################################################################################################

__all__ = ['mega2560']

####################################################################################################

class _Mega2560(object):

    __mapping__ = (
        {'pin':   1, 'name': ('PG5', 'OC0B'),                   'mapped':('Digital4', 'PWM4',)},
        {'pin':   2, 'name': ('PE0', 'RXD0', 'PCINT8'),         'mapped':('Digital0', 'RX0)',)},
        {'pin':   3, 'name': ('PE1', 'TXD0'),                   'mapped':('Digital1', 'TX0',)},
        {'pin':   4, 'name': ('PE2', 'XCK0', 'AIN0'),           'mapped':()},
        {'pin':   5, 'name': ('PE3', 'OC3A', 'AIN1'),           'mapped':('Digital5', 'PWM5',)},
        {'pin':   6, 'name': ('PE4', 'OC3B', 'INT4'),           'mapped':('Digital2', 'PWM2',)},
        {'pin':   7, 'name': ('PE5', 'OC3C', 'INT5'),           'mapped':('Digital3', 'PWM3',)},
        {'pin':   8, 'name': ('PE6', 'T3', 'INT6'),             'mapped':()},
        {'pin':   9, 'name': ('PE7', 'CLKO', 'ICP3', 'INT7'),   'mapped':()},
        {'pin':  10, 'name': ('VCC',),                          'mapped':('VCC',)},
        {'pin':  11, 'name': ('GND',),                          'mapped':('GND',)},
        {'pin':  12, 'name': ('PH0', 'RXD2'),                   'mapped':('Digital17', 'RX2',)},
        {'pin':  13, 'name': ('PH1', 'TXD2'),                   'mapped':('Digital16', 'TX2',)},
        {'pin':  14, 'name': ('PH2', 'XCK2'),                   'mapped':()},
        {'pin':  15, 'name': ('PH3', 'OC4A'),                   'mapped':('Digital6', 'PWM6',)},
        {'pin':  16, 'name': ('PH4', 'OC4B'),                   'mapped':('Digital7', 'PWM7',)},
        {'pin':  17, 'name': ('PH5', 'OC4C'),                   'mapped':('Digital8', 'PWM8',)},
        {'pin':  18, 'name': ('PH6', 'OC2B'),                   'mapped':('Digital9', 'PWM9',)},
        {'pin':  19, 'name': ('PB0', 'SS', 'PCINT0'),           'mapped':('Digital53', 'SS',)},
        {'pin':  20, 'name': ('PB1', 'SCK', 'PCINT1'),          'mapped':('Digital52', 'SCK',)},
        {'pin':  21, 'name': ('PB2', 'MOSI', 'PCINT2'),         'mapped':('Digital51', 'MOSI',)},
        {'pin':  22, 'name': ('PB3', 'MISO', 'PCINT3'),         'mapped':('Digital50', 'MISO',)},
        {'pin':  23, 'name': ('PB4', 'OC2A', 'PCINT4'),         'mapped':('Digital10', 'PWM10',)},
        {'pin':  24, 'name': ('PB5', 'OC1A', 'PCINT5'),         'mapped':('Digital11', 'PWM11',)},
        {'pin':  25, 'name': ('PB6', 'OC1B', 'PCINT6'),         'mapped':('Digital12', 'PWM12',)},
        {'pin':  26, 'name': ('PB7', 'OC0A', 'OC1C', 'PCINT7'), 'mapped':('Digital13', 'PWM13',)},
        {'pin':  27, 'name': ('PH7', 'T4'),                     'mapped':()},
        {'pin':  28, 'name': ('PG3', 'TOSC2'),                  'mapped':()},
        {'pin':  29, 'name': ('PG4', 'TOSC1'),                  'mapped':()},
        {'pin':  30, 'name': ('RESET',),                        'mapped':('RESET',)},
        {'pin':  31, 'name': ('VCC',),                          'mapped':('VCC',)},
        {'pin':  32, 'name': ('GND',),                          'mapped':('GND',)},
        {'pin':  33, 'name': ('XTAL2',),                        'mapped':('XTAL2',)},
        {'pin':  34, 'name': ('XTAL1',),                        'mapped':('XTAL1',)},
        {'pin':  35, 'name': ('PL0', 'ICP4'),                   'mapped':('Digital49',)},
        {'pin':  36, 'name': ('PL1', 'ICP5'),                   'mapped':('Digital48',)},
        {'pin':  37, 'name': ('PL2', 'T5'),                     'mapped':('Digital47',)},
        {'pin':  38, 'name': ('PL3', 'OC5A'),                   'mapped':('Digital46', 'PWM46',)},
        {'pin':  39, 'name': ('PL4', 'OC5B'),                   'mapped':('Digital45', 'PWM45',)},
        {'pin':  40, 'name': ('PL5', 'OC5C'),                   'mapped':('Digital44', 'PWM44',)},
        {'pin':  41, 'name': ('PL6',),                          'mapped':('Digital43',)},
        {'pin':  42, 'name': ('PL7',),                          'mapped':('Digital42',)},
        {'pin':  43, 'name': ('PD0', 'SCL', 'INT0'),            'mapped':('Digital21', 'SCL',)},
        {'pin':  44, 'name': ('PD1', 'SDA', 'INT1'),            'mapped':('Digital20', 'SDA',)},
        {'pin':  45, 'name': ('PD2', 'RXDI', 'INT2'),           'mapped':('Digital19', 'RX1',)},
        {'pin':  46, 'name': ('PD3', 'TXD1', 'INT3'),           'mapped':('Digital18', 'TX1',)},
        {'pin':  47, 'name': ('PD4', 'ICP1'),                   'mapped':()},
        {'pin':  48, 'name': ('PD5', 'XCK1'),                   'mapped':()},
        {'pin':  49, 'name': ('PD6', 'T1'),                     'mapped':()},
        {'pin':  50, 'name': ('PD7', 'T0'),                     'mapped':('Digital38',)},
        {'pin':  51, 'name': ('PG0', 'WR'),                     'mapped':('Digital41',)},
        {'pin':  52, 'name': ('PG1', 'RD'),                     'mapped':('Digital40',)},
        {'pin':  53, 'name': ('PC0', 'A8'),                     'mapped':('Digital37',)},
        {'pin':  54, 'name': ('PC1', 'A9'),                     'mapped':('Digital36',)},
        {'pin':  55, 'name': ('PC2', 'A10'),                    'mapped':('Digital35',)},
        {'pin':  56, 'name': ('PC3', 'A11'),                    'mapped':('Digital34',)},
        {'pin':  57, 'name': ('PC4', 'A12'),                    'mapped':('Digital33',)},
        {'pin':  58, 'name': ('PC5', 'A13'),                    'mapped':('Digital32',)},
        {'pin':  59, 'name': ('PC6', 'A14'),                    'mapped':('Digital31',)},
        {'pin':  60, 'name': ('PC7', 'A15'),                    'mapped':('Digital30',)},
        {'pin':  61, 'name': ('VCC',),                          'mapped':('VCC',)},
        {'pin':  62, 'name': ('GND',),                          'mapped':('GND',)},
        {'pin':  63, 'name': ('PJ0', 'RXD3', 'PCINT9'),         'mapped':('Digital15', 'RX3',)},
        {'pin':  64, 'name': ('PJ1', 'TXD3', 'PCINT10'),        'mapped':('Digital14', 'TX3',)},
        {'pin':  65, 'name': ('PJ2', 'XCK3', 'PCINT11'),        'mapped':()},
        {'pin':  66, 'name': ('PJ3', 'PCINT12'),                'mapped':()},
        {'pin':  67, 'name': ('PJ4', 'PCINT13'),                'mapped':()},
        {'pin':  68, 'name': ('PJ5', 'PCINT14'),                'mapped':()},
        {'pin':  69, 'name': ('PJ6', 'PCINT15'),                'mapped':()},
        {'pin':  70, 'name': ('PG2', 'ALE'),                    'mapped':('Digital39',)},
        {'pin':  71, 'name': ('PA7', 'AD7'),                    'mapped':('Digital29',)},
        {'pin':  72, 'name': ('PA6', 'AD6'),                    'mapped':('Digital28',)},
        {'pin':  73, 'name': ('PA5', 'AD5'),                    'mapped':('Digital27',)},
        {'pin':  74, 'name': ('PA4', 'AD4'),                    'mapped':('Digital26',)},
        {'pin':  75, 'name': ('PA3', 'AD3'),                    'mapped':('Digital25',)},
        {'pin':  76, 'name': ('PA2', 'AD2'),                    'mapped':('Digital24',)},
        {'pin':  77, 'name': ('PA1', 'AD1'),                    'mapped':('Digital23',)},
        {'pin':  78, 'name': ('PA0', 'AD0'),                    'mapped':('Digital22',)},
        {'pin':  79, 'name': ('PJ7',),                          'mapped':()},
        {'pin':  80, 'name': ('VCC',),                          'mapped':('VCC',)},
        {'pin':  81, 'name': ('GND',),                          'mapped':('GND',)},
        {'pin':  82, 'name': ('PK7', 'ADC15', 'PCINT23'),       'mapped':('Analog15',)},
        {'pin':  83, 'name': ('PK6', 'ADC14', 'PCINT22'),       'mapped':('Analog14',)},
        {'pin':  84, 'name': ('PK5', 'ADC13', 'PCINT21'),       'mapped':('Analog13',)},
        {'pin':  85, 'name': ('PK4', 'ADC12', 'PCINT20'),       'mapped':('Analog12',)},
        {'pin':  86, 'name': ('PK3', 'ADC11', 'PCINT19'),       'mapped':('Analog11',)},
        {'pin':  87, 'name': ('PK2', 'ADC10', 'PCINT18'),       'mapped':('Analog10',)},
        {'pin':  88, 'name': ('PK1', 'ADC9', 'PCINT17'),        'mapped':('Analog9',)},
        {'pin':  89, 'name': ('PK0', 'ADC8', 'PCINT16'),        'mapped':('Analog8',)},
        {'pin':  90, 'name': ('PF7', 'ADC7'),                   'mapped':('Analog7',)},
        {'pin':  91, 'name': ('PF6', 'ADC6'),                   'mapped':('Analog6',)},
        {'pin':  92, 'name': ('PF5', 'ADC5', 'TMS'),            'mapped':('Analog5',)},
        {'pin':  93, 'name': ('PF4', 'ADC4', 'TMK'),            'mapped':('Analog4',)},
        {'pin':  94, 'name': ('PF3', 'ADC3'),                   'mapped':('Analog3',)},
        {'pin':  95, 'name': ('PF2', 'ADC2'),                   'mapped':('Analog2',)},
        {'pin':  96, 'name': ('PF1', 'ADC1'),                   'mapped':('Analog1',)},
        {'pin':  97, 'name': ('PF0', 'ADC0'),                   'mapped':('Analog0',)},
        {'pin':  98, 'name': ('AREF',),                         'mapped':('AREF',)},
        {'pin':  99, 'name': ('GND',),                          'mapped':('GND',)},
        {'pin': 100, 'name': ('AVCC',),                         'mapped':('VCC',)},
    )

    ##############################################

    def __init__(self):

        self._avr_map = {}
        self._ardiuno_map = {}
        for pin_mapping  in self.__mapping__:
            for name in pin_mapping['name']:
                self._avr_map[name] = pin_mapping
            for name in pin_mapping['mapped']:
                self._ardiuno_map[name] = pin_mapping
                
    ##############################################    

    def to_port(self, name):
        pin_mapping = self._ardiuno_map[name]
        first_name = pin_mapping['name'][0]
        if first_name.startswith('P'):
            return first_name[1], int(first_name[2])
        else:
            return None

####################################################################################################
        
mega2560 = _Mega2560()
        
####################################################################################################
# 
# End
# 
####################################################################################################
