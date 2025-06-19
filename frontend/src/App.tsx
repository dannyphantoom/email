import * as React from 'react';
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { Toaster } from 'react-hot-toast';
import { useAuthStore } from './stores/authStore';
import Login from './pages/Login';
import Register from './pages/Register';
import Messenger from './pages/Messenger';
import './index.css';

const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      retry: 1,
      refetchOnWindowFocus: false,
    },
  },
});

function App() {
  const { isAuthenticated, logout } = useAuthStore();
  
  console.log('App component rendering, isAuthenticated:', isAuthenticated);

  return (
    <QueryClientProvider client={queryClient}>
      <Router>
        <div className="min-h-screen bg-gradient-to-br from-dark-950 via-dark-900 to-cockpit-950">
          {/* Temporary debug button */}
          {isAuthenticated && (
            <div className="absolute top-4 right-4 z-50">
              <button 
                onClick={logout}
                className="bg-red-600 hover:bg-red-700 text-white px-4 py-2 rounded-lg text-sm"
              >
                Clear Auth (Debug)
              </button>
            </div>
          )}
          
          <Routes>
            <Route 
              path="/login" 
              element={
                <div>
                  {console.log('Rendering login route')}
                  {isAuthenticated ? <Navigate to="/messenger" /> : <Login />}
                </div>
              } 
            />
            <Route 
              path="/register" 
              element={
                <div>
                  {console.log('Rendering register route')}
                  {isAuthenticated ? <Navigate to="/messenger" /> : <Register />}
                </div>
              } 
            />
            <Route 
              path="/messenger/*" 
              element={
                <div>
                  {console.log('Rendering messenger route')}
                  {isAuthenticated ? <Messenger /> : <Navigate to="/login" />}
                </div>
              } 
            />
            <Route 
              path="/" 
              element={
                <div>
                  {console.log('Rendering root route, redirecting to:', isAuthenticated ? "/messenger" : "/login")}
                  <Navigate to={isAuthenticated ? "/messenger" : "/login"} />
                </div>
              } 
            />
          </Routes>
        </div>
        
        <Toaster
          position="top-right"
          toastOptions={{
            duration: 4000,
            style: {
              background: '#1a1d23',
              color: '#fff',
              border: '1px solid #333945',
            },
            success: {
              iconTheme: {
                primary: '#10b981',
                secondary: '#fff',
              },
            },
            error: {
              iconTheme: {
                primary: '#ef4444',
                secondary: '#fff',
              },
            },
          }}
        />
      </Router>
    </QueryClientProvider>
  );
}

export default App; 