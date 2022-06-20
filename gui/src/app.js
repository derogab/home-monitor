import React, { useState } from "react";
import { BrowserRouter, Route, Switch, Redirect } from "react-router-dom";

import "@fortawesome/fontawesome-free/css/all.min.css";
import "assets/styles/tailwind.css";
import "assets/styles/custom.css";

// components

import Navbar from "components/Navbars/Navbar.js";
import Sidebar from "components/Sidebar/Sidebar.js";
import HeaderStats from "components/Headers/HeaderStats.js";
import Footer from "components/Footers/Footer.js";

// views

import Dashboard from "views/Dashboard.js";
import Settings from "views/Settings.js";
import Tables from "views/Tables.js";


export default function App() {

  // Init stateful
  const [deviceSelected, setDevice] = useState(null);

  // Render app
  return (
    <>
      <BrowserRouter>
          <Sidebar />
          <div className="relative md:ml-64 bg-blueGray-100">
              <Navbar />
              {/* Header */}
              <HeaderStats deviceSelected={deviceSelected} setDevice={setDevice} />
              <div className="px-4 md:px-10 mx-auto w-full -m-24">
              <Switch deviceSelected={deviceSelected}>
                <Route path="/dashboard" exact component={Dashboard} />
                <Route path="/settings" exact component={Settings} />
                <Route path="/tables" exact component={Tables} />
                <Redirect from="/" to="/dashboard" />
              </Switch>
              <Footer />
              </div>
          </div>
      </BrowserRouter>
    </>
  );
}