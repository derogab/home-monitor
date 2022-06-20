import React, { useState, useEffect } from "react";
import PropTypes from "prop-types";
import AsyncSelect from 'react-select/async';


const promiseOptions = () =>
  new Promise((resolve) => {

    fetch('http://localhost:3001/devices', {
      method: 'GET',
      headers: {
        "Content-Type": "application/json",
        "Access-Control-Allow-Origin": "*", 
        
      },
    })
    .then((response) => {
      // Empty array if no data
      if (!response.ok) return [];
      // Parse data
      return response.json();
    })
    .then((data) => {
      // Check if there are data
      if (!data) return [];
      // Get sensors
      const sensors = data.devices || [];
      // Check if there are sensors
      if (!sensors) return [];
      // Fix data
      let selectable = [];
      for (let i = 0; i < sensors.length; i++) {
        const sensor = sensors[i];
        // Append to selectable
        selectable.push({
          value: sensor.mac,
          label: sensor.name + ' (' + sensor.mac + ')',
        });
      }
      console.log('Selectable: ', selectable);
      // Resolve promise
      resolve(selectable);
    })
    .catch(function (err) {
      console.log("Unable to fetch - ", err);
    });

});

const getStatusClassName = function(status) {
  if (status === 'unknown') return "";
  else if (status) return "is-alive";
  else return "is-not-alive";
};

const CardSelector = ({
  statId,
  statTitle,
  deviceSelected,
  onChangeCallback,
}) => {
  // Init stateful
  const [value, setValue] = useState(1);
  const [isDeviceAlive, setAliveStatus] = useState('unknown');

  // Update data
  useEffect(() => {
    const interval = setInterval(() => {
      
      if (deviceSelected) fetch('http://localhost:3001/devices/' + deviceSelected, {
        method: 'GET',
        headers: {
          "Content-Type": "application/json",
          "Access-Control-Allow-Origin": "*",
        },
      })
      .then((response) => {
        // Empty array if no data
        if (!response.ok) return;
        // Parse data
        return response.json();
      })
      .then((data) => {
        console.log(data);
        // Check if there are data
        if (!data) return;
        // Update state
        setAliveStatus(data.value);
        // Log
        console.log('Update value of ' + deviceSelected + ' status: ' + (data.value ? "YES" : "NO"));
      })
      .catch(function (err) {
        console.log("Unable to fetch - ", err);
      });

    }, 3000);
  
    return () => {

      clearInterval(interval);

    };

  }, [deviceSelected]);

  // On change function
  const onChangeSelectedOption = (e) => {
    // Return, if no value
    if (!e) return;
    // Get value
    const selectedOption = e.value; 
    // Log
    console.log('CardSelector / Selected: ', selectedOption);
    // Callback
    onChangeCallback(selectedOption);
  };

  // Print data
  return (
    <>
      <div id={statId} className={
          "relative flex flex-col min-w-0 break-words bg-white rounded mb-6 xl:mb-0 shadow-lg fixed-min-height " +
          (getStatusClassName(isDeviceAlive))
        } onClick={() => setValue(!value)}>
        <div className="flex-auto p-4">
          <div className="flex flex-wrap">
            <div className="relative w-full pr-4 max-w-full flex-grow flex-1">
              <h5 className={
                "text-blueGray-400 uppercase font-bold text-xs " +
                (getStatusClassName(isDeviceAlive))
              }>
                {statTitle}
              </h5>
              <span className="font-semibold text-xl text-blueGray-700">
                <AsyncSelect cacheOptions defaultOptions loadOptions={promiseOptions} onChange={onChangeSelectedOption} className="device-selector" />
              </span>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};

CardSelector.defaultProps = {
  statId: '1234',
  statTitle: "Example",
  deviceSelected: "abcd1234",
  onChangeCallback: null
};

CardSelector.propTypes = {
  statId: PropTypes.string,
  statTitle: PropTypes.string,
  deviceSelected: PropTypes.string,
  onChangeCallback: PropTypes.func
};

export default CardSelector;
