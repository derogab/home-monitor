import React, { useState } from "react";
import PropTypes from "prop-types";
import AsyncSelect from 'react-select/async';

const getDevices = () => {
  return [
    { value: 'chocolate', label: 'Chocolate' },
    { value: 'strawberry', label: 'Strawberry' },
    { value: 'vanilla', label: 'Vanilla' }
  ];
};

const promiseOptions = () =>
  new Promise((resolve) => {
    setTimeout(() => {
      console.log("promiseOptions...");
      resolve(getDevices());
    }, 1000);
});

const CardSelector = ({
  statId,
  statTitle,
}) => {
  // Init stateful
  const [value, setValue] = useState(1);

  // Print data
  return (
    <>
      <div id={statId} className="relative flex flex-col min-w-0 break-words bg-white rounded mb-6 xl:mb-0 shadow-lg fixed-min-height" onClick={() => setValue(!value)}>
        <div className="flex-auto p-4">
          <div className="flex flex-wrap">
            <div className="relative w-full pr-4 max-w-full flex-grow flex-1">
              <h5 className="text-blueGray-400 uppercase font-bold text-xs">
                {statTitle}
              </h5>
              <span className="font-semibold text-xl text-blueGray-700">
                <AsyncSelect cacheOptions defaultOptions loadOptions={promiseOptions} />
              </span>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};

CardSelector.defaultProps = {
  statTitle: "Example",
  statIconName: "far fa-chart-bar",
  statIconColor: "bg-red-500",
};

CardSelector.propTypes = {
  statTitle: PropTypes.string,
  // can be any of the text color utilities
  // from tailwindcss
  statIconName: PropTypes.string,
  // can be any of the background color utilities
  // from tailwindcss
  statIconColor: PropTypes.string,
};

export default CardSelector;
