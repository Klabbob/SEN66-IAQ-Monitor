import serial
import time
from datetime import datetime
import uuid
import platform
import json
import os

class EDFWriter:
    def __init__(self, port, baudrate=115200, output_dir="Temperature Parameter Tuning"):
        self.serial = serial.Serial(port, baudrate)
        self.output_dir = output_dir
        self.session_id = str(uuid.uuid4())
        self.group_id = str(uuid.uuid4())
        self.sensor_id = "1241021161703511"  # Default sensor ID, can be updated
        
        # Create output directory if it doesn't exist
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
            
        # VOC and NOx algorithm parameters
        self.voc_params = {
            "algorithm_version": "3.1.0",
            "index_offset": 100,
            "learning_time_offset_hours": 12,
            "learning_time_gain_hours": 12,
            "gating_max_duration_minutes": 180,
            "std_initial": 50,
            "gain_factor": 230,
            "format": 2
        }
        
        self.nox_params = {
            "algorithm_version": "3.1.0",
            "index_offset": 1,
            "learning_time_offset_hours": 12,
            "learning_time_gain_hours": 12,
            "gating_max_duration_minutes": 720,
            "std_initial": 50,
            "gain_factor": 230,
            "format": 2
        }

    def generate_header(self):
        current_time = datetime.now().isoformat()
        hostname = platform.node()
        os_info = f"{platform.system()}-{platform.release()}-{platform.version()}"
        
        header = f"""# EdfVersion=4.0
# Date={current_time}
# MeasurementSession={self.session_id}
# MeasurementGroup={self.group_id}
# HostName={hostname}
# ApplicationName=ControlCenter
# ApplicationVersion=1.45.0
# OperatingSystem={os_info}
# SensorId={self.sensor_id}
# voc_algorithm_parameters={json.dumps(self.voc_params)}
# nox_algorithm_parameters={json.dumps(self.nox_params)}
# Type=float64,Format=.3f,Unit=s\tType=string,Format=\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Mass_Concentration_1p0,Unit=ug/m³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Mass_Concentration_2p5,Unit=ug/m³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Mass_Concentration_4p0,Unit=ug/m³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Mass_Concentration_10p,Unit=ug/m³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Relative_Humidity,Unit=%RH,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Temperature,Unit=°C,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=VOC_Index,Unit=VOC Index,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=NOx_Index,Unit=NOx Index,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Carbon_Dioxide,Unit=ppm,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Number_Concentration_0p5,Unit=N/cm³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Number_Concentration_1p0,Unit=N/cm³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Number_Concentration_2p5,Unit=N/cm³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Number_Concentration_4p0,Unit=N/cm³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Number_Concentration_10p,Unit=N/cm³,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Relative_Humidity_Raw_Signal,Unit=%RH,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Temperature_Raw_Signal,Unit=°C,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=VOC_Raw_Signal,Unit=ticks,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=NOx_Raw_Signal,Unit=ticks,Product=SEN66,Measurement_Frequency=1.0Hz\tType=float,Format=.6f,Device=SensorBridge_(7d4776),Device_Serial_Number=SensorBridge-260058000151313137343431,Port=2,SensorName=SEN66_green,Sensor_Serial_Number={self.sensor_id},Signal=Raw_Carbon_Dioxide,Unit=ppm,Product=SEN66,Measurement_Frequency=1.0Hz
Epoch_UTC\tLocal_Date_Time\tMassConc_1p0_SEN66_green_{self.sensor_id}\tMassConc_2p5_SEN66_green_{self.sensor_id}\tMassConc_4p0_SEN66_green_{self.sensor_id}\tMassConc_10p_SEN66_green_{self.sensor_id}\tRH_SEN66_green_{self.sensor_id}\tT_SEN66_green_{self.sensor_id}\tIndex_VOC_SEN66_green_{self.sensor_id}\tIndex_NOx_SEN66_green_{self.sensor_id}\tC_CO2_SEN66_green_{self.sensor_id}\tNumbConc_0p5_SEN66_green_{self.sensor_id}\tNumbConc_1p0_SEN66_green_{self.sensor_id}\tNumbConc_2p5_SEN66_green_{self.sensor_id}\tNumbConc_4p0_SEN66_green_{self.sensor_id}\tNumbConc_10p_SEN66_green_{self.sensor_id}\tRH_RAW_SEN66_green_{self.sensor_id}\tT_RAW_SEN66_green_{self.sensor_id}\tSRAW_VOC_SEN66_green_{self.sensor_id}\tSRAW_NOx_SEN66_green_{self.sensor_id}\tCRAW_CO2_SEN66_green_{self.sensor_id}
"""
        return header

    def parse_serial_line(self, line):
        # Split the line into values
        values = line.strip().split('\t')
        if len(values) != 13:  # Expected number of values
            return None
            
        # Convert values to appropriate types
        try:
            pm1p0 = float(values[0])
            pm2p5 = float(values[1])
            pm4p0 = float(values[2])
            pm10p0 = float(values[3])
            humidity = float(values[4])
            temperature = float(values[5])
            voc_index = float(values[6])
            nox_index = float(values[7])
            co2 = float(values[8])
            raw_humidity = float(values[9]) * 100  # Convert to raw value
            raw_temperature = float(values[10]) * 200  # Convert to raw value
            raw_voc = float(values[11])
            raw_nox = float(values[12])
            raw_co2 = float(values[8])  # Same as CO2 for raw value
            
            # Calculate number concentrations (using same values as mass concentrations for simplicity)
            numb_conc_0p5 = pm1p0 * 7  # Approximate conversion
            numb_conc_1p0 = pm1p0 * 9
            numb_conc_2p5 = pm2p5 * 6.8
            numb_conc_4p0 = pm4p0 * 6.8
            numb_conc_10p = pm10p0 * 6.8
            
            return {
                'pm1p0': pm1p0,
                'pm2p5': pm2p5,
                'pm4p0': pm4p0,
                'pm10p0': pm10p0,
                'humidity': humidity,
                'temperature': temperature,
                'voc_index': voc_index,
                'nox_index': nox_index,
                'co2': co2,
                'numb_conc_0p5': numb_conc_0p5,
                'numb_conc_1p0': numb_conc_1p0,
                'numb_conc_2p5': numb_conc_2p5,
                'numb_conc_4p0': numb_conc_4p0,
                'numb_conc_10p': numb_conc_10p,
                'raw_humidity': raw_humidity,
                'raw_temperature': raw_temperature,
                'raw_voc': raw_voc,
                'raw_nox': raw_nox,
                'raw_co2': raw_co2
            }
        except (ValueError, IndexError):
            return None

    def write_data_line(self, data, timestamp):
        local_time = datetime.fromtimestamp(timestamp).isoformat()
        
        line = f"{timestamp:.6f}\t{local_time}\t"
        line += f"{data['pm1p0']:.6f}\t{data['pm2p5']:.6f}\t{data['pm4p0']:.6f}\t{data['pm10p0']:.6f}\t"
        line += f"{data['humidity']:.6f}\t{data['temperature']:.6f}\t{data['voc_index']:.6f}\t{data['nox_index']:.6f}\t"
        line += f"{data['co2']:.6f}\t{data['numb_conc_0p5']:.6f}\t{data['numb_conc_1p0']:.6f}\t"
        line += f"{data['numb_conc_2p5']:.6f}\t{data['numb_conc_4p0']:.6f}\t{data['numb_conc_10p']:.6f}\t"
        line += f"{data['raw_humidity']:.6f}\t{data['raw_temperature']:.6f}\t{data['raw_voc']:.6f}\t"
        line += f"{data['raw_nox']:.6f}\t{data['raw_co2']:.6f}\n"
        
        return line

    def run(self):
        # Generate filename with current timestamp
        timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
        filename = f"{self.output_dir}/{timestamp}-SEN66_green_{self.sensor_id}.edf"
        
        with open(filename, 'w') as f:
            # Write header
            f.write(self.generate_header())
            
            print(f"Recording data to {filename}")
            print("Press Ctrl+C to stop recording")
            
            try:
                while True:
                    if self.serial.in_waiting:
                        line = self.serial.readline().decode('utf-8').strip()
                        if line.startswith("PM1.0"):  # Skip header line
                            continue
                            
                        data = self.parse_serial_line(line)
                        if data:
                            timestamp = time.time()
                            data_line = self.write_data_line(data, timestamp)
                            f.write(data_line)
                            f.flush()  # Ensure data is written to disk
                            
            except KeyboardInterrupt:
                print("\nRecording stopped")
            finally:
                self.serial.close()

if __name__ == "__main__":
    # Update the port to match your serial connection
    writer = EDFWriter(port="COM3")  # Change this to your actual COM port
    writer.run() 